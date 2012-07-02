/*
 * vm.c
 * Virtual machine.
 */

#include "lib.h"
#include "process.h"
#include "stream.h"
#include "file.h"
#include "vmproc.h"

#include "vm.h"
#include "value.h"
#include "portray.h"
#include "save.h"

#include "instrenum.h"

#ifdef DEBUG
#include "cmdline.h"	/* for process_err */
#include "render.h"
#define VM_DEBUG(x) 	process_render(process_err, "EXEC: %s\n", # x);
#define VM_DEBUG_PC()	process_render(process_err, "VM PC: %04d --> ", pc);
#define	VM_DUMP_AR()							\
	if (value_is_null(&ar)) {					\
		process_render(process_err, "(NO AR) ");		\
	} else {							\
		process_render(process_err, "AR: ");			\
		value_portray(process_err, &ar);			\
		process_render(process_err, " ");			\
	}
#else
#define VM_DEBUG(x)
#define VM_DEBUG_PC()
#define VM_DUMP_AR()
#endif

#ifdef DIRECT_THREADING

#include "instrtab.h"

#define VM_TOP()		TOP:
#define VM_BEGIN_DISPATCH()	goto *value_tuple_fetch_label(code, pc);
#define VM_END_DISPATCH()
#define VM_OPLAB(x)		LABEL_ ## x: VM_DEBUG(x)
#define VM_NEXT()		goto TOP;
#define VM_STOP()		cycles = 1; goto TOP;

#else

#define VM_TOP()
#define VM_BEGIN_DISPATCH()	switch (value_tuple_fetch_integer(code, pc)) {
#define VM_END_DISPATCH()	}
#define VM_OPLAB(x)		case x: VM_DEBUG(x)
#define VM_NEXT()		break;
#define VM_STOP()		cycles = 1; break;

#endif

#define POP_VALUE()	value_ar_pop(&ar)
#define PUSH_VALUE(v)	value_ar_push(&ar, v)

#define GET_VALUE(i)	value_ar_push(&ar,	\
			    value_tuple_fetch(&ar,	\
			    i + AR_HEADER_SIZE))
#define SET_VALUE(i)	value_tuple_store(&ar,	\
			    i + AR_HEADER_SIZE,	\
			    value_ar_pop(&ar))

#define XFER_VALUES(from, to, count) value_ar_xfer(from, to, count)

#define	IMM_VAL()	(value_tuple_fetch(code, pc))
#define	IMM_INT()	(value_tuple_fetch_integer(code, pc))
#define	IMM_ADDR()	(value_tuple_fetch_integer(code, pc))

void
vm_run(struct value *vm, struct process *self, unsigned int cycles)
{
	struct value t1;    /* temporary */

	struct value *a;    /* register, generally used for 1st argument */
	struct value *b;    /* register, generally used for 2nd argument */
	struct value *v;    /* register, generally used for result */

	struct value ar;   /* contains currently active activation record */
	struct value *code; /* tuple containing VM instructions */

	unsigned int pc;   /* pointer into code to currently exec instr */

#ifdef DIRECT_THREADING
	#include "instrlab.h"

	if (!value_get_boolean(value_tuple_fetch(vm, VM_IS_DIRECT))) {
		struct opcode_entry *oe;
		enum opcode opcode;

		/* convert opcodes to labels */
		pc = 0;
		code = value_tuple_fetch(vm, VM_CODE);
		a = value_tuple_fetch(code, pc);
		opcode = (enum opcode)value_get_integer(a);
		while (opcode != INSTR_EOF) {
			value_label_set(a, instr_label[value_get_integer(a)]);
#ifdef DEBUG
			process_render(process_err, "At %d, replaced %d with ", pc, opcode);
			value_portray(process_err, value_tuple_fetch(code, pc));
			process_render(process_err, "\n");
#endif
			pc += opcode_table[opcode].arity;
			pc++;
			a = value_tuple_fetch(code, pc);
			opcode = (enum opcode)value_get_integer(a);
		}
		value_tuple_store(vm, VM_IS_DIRECT, &VTRUE);
	}
#endif

	value_copy(&ar, value_tuple_fetch(vm, VM_AR));
	code = value_tuple_fetch(vm, VM_CODE);
	pc = value_tuple_fetch_integer(vm, VM_PC);
	pc--;

	for (;;) {
		VM_TOP()

		pc++;
		if (--cycles == 0) break;
		VM_DEBUG_PC()
		VM_DUMP_AR()

		VM_BEGIN_DISPATCH()
		
		/*** STACK MANIPULATION ***/

		/*
		 % PUSH v : -> v
		 * Push the immediate value onto the stack.
		 */
		VM_OPLAB(INSTR_PUSH)
			pc++;
			PUSH_VALUE(IMM_VAL());
			VM_NEXT()

		/*
		 % POP : v ->
		 * Pop a value off the stack and discard it.
		 */
		VM_OPLAB(INSTR_POP)
			POP_VALUE();
			VM_NEXT()

		/*
		 % GET : i -> v
		 * Push the value of a local variable, given
		 * by the index popped from the stack, onto the
		 * stack.  i = 0 indicates the first local,
		 * which is stored at the bottom of the stack.
		 */
		VM_OPLAB(INSTR_GET)
			a = POP_VALUE();
			GET_VALUE(value_get_integer(a));
			VM_NEXT()

		/*
		 % SET : v i ->
		 * Alter the value of a local variable, given
		 * by the index popped from the stack, to be
		 * the value subsequently popped from the stack.
		 */
		VM_OPLAB(INSTR_SET)
			a = POP_VALUE();
			SET_VALUE(value_get_integer(a));
			VM_NEXT()

		/*
		 % GETI i : -> v
		 * Push the value of a local variable, given
		 * by the immediate integer index, onto the stack.
		 */
		VM_OPLAB(INSTR_GETI)
			pc++;
			GET_VALUE(IMM_INT());
			VM_NEXT()

		/*
		 % SETI i : v ->
		 * Alter the value of a local variable, given
		 * by the immediate integer index, to be
		 * the value popped from the stack.
		 */
		VM_OPLAB(INSTR_SETI)
			pc++;
			SET_VALUE(IMM_INT());
			VM_NEXT()

		/*** TUPLE OPERATIONS ***/

		/*
		 % NEW_TUPLE i : v -> t
		 * Push a new, empty tuple onto the stack.
		 * The immediate integer gives the size.
		 * The value on the stack gives the tuple's tag.
		 */
		VM_OPLAB(INSTR_NEW_TUPLE)
			pc++;
			value_tuple_new(&t1, POP_VALUE(), IMM_INT());
			PUSH_VALUE(&t1);
			VM_NEXT()

		/*
		 % FETCH_TUPLE : i t -> v
		 * Pop a tuple value from the stack, then
		 * an integer index value, then push the
		 * value stored at that index in the tuple.
		 */
		VM_OPLAB(INSTR_FETCH_TUPLE)
			a = POP_VALUE(); /* tuple */
			b = POP_VALUE(); /* index */
			PUSH_VALUE(value_tuple_fetch(a, value_get_integer(b)));
			VM_NEXT()

		/*
		 % STORE_TUPLE : v i t ->
		 * Pop a tuple value from the stack, then
		 * an integer index value, then a target value;
		 * store the target in the tuple at that index.
		 */
		VM_OPLAB(INSTR_STORE_TUPLE)
			a = POP_VALUE(); /* tuple */
			b = POP_VALUE(); /* index */
			v = POP_VALUE(); /* value */
			value_tuple_store(a, value_get_integer(b), v);
			VM_NEXT()

		/*** DICTIONARY OPERATIONS ***/

		/*
		 % NEW_DICT i : -> d
		 * Push a new, empty dictionary onto the stack.
		 * The immediate integer gives the load factor.
		 */
		VM_OPLAB(INSTR_NEW_DICT)
			pc++;
			value_dict_new(&t1, IMM_INT());
			PUSH_VALUE(&t1);
			VM_NEXT()

		/*
		 % FETCH_DICT : k d -> v
		 * Pop a dictionary value from the stack, then
		 * a key value, then push the associated stored
		 * value retrieved from the dictionary onto the
		 * stack.
		 */
		VM_OPLAB(INSTR_FETCH_DICT)
			a = POP_VALUE(); /* dictionary */
			b = POP_VALUE(); /* key */
			PUSH_VALUE(value_dict_fetch(a, b));
			VM_NEXT()

		/*
		 % STORE_DICT : v k d ->
		 * Pop a dictionary value from the stack, then
		 * a key value, then a target value; associate
		 * the key with the target in the dictionary.
		 */
		VM_OPLAB(INSTR_STORE_DICT)
			a = POP_VALUE(); /* dictionary */
			b = POP_VALUE(); /* key */
			v = POP_VALUE(); /* value */
			value_dict_store(a, b, v);
			VM_NEXT()

		/*** BOOLEAN OPERATORS ***/

		/*
		 % NOT : b -> b
		 * Pop a boolean, and push Boolean NOT of it.
		 */
		VM_OPLAB(INSTR_NOT)
			a = POP_VALUE();
			value_boolean_set(&t1, !value_get_boolean(a));
			PUSH_VALUE(&t1);
			VM_NEXT()

		/*
		 % AND : b b -> b
		 * Pop two booleans, and push Boolean AND of them.
		 */
		VM_OPLAB(INSTR_AND)
			b = POP_VALUE();
			a = POP_VALUE();
			value_boolean_set(&t1,
			    value_get_boolean(a) && value_get_boolean(b)
			);
			PUSH_VALUE(&t1);
			VM_NEXT()

		/*
		 % OR : b b -> b
		 * Pop two booleans, and push Boolean OR of them.
		 */
		VM_OPLAB(INSTR_OR)
			b = POP_VALUE();
			a = POP_VALUE();
			value_boolean_set(&t1,
			    value_get_boolean(a) || value_get_boolean(b)
			);
			PUSH_VALUE(&t1);
			VM_NEXT()

		/*** COMPARISON OPERATORS ***/

		/*
		 % EQU : v v -> b
		 * Pop two value, and push a new boolean value;
		 * true if the two values are equal, false if not.
		 */
		VM_OPLAB(INSTR_EQU)
			b = POP_VALUE();
			a = POP_VALUE();
			value_boolean_set(&t1,
			    value_equal(a, b)
			);
			PUSH_VALUE(&t1);
			VM_NEXT()

		/*
		 % NEQ : v v -> b
		 * Shorthand for EQU NOT.
		 */
		VM_OPLAB(INSTR_NEQ)
			b = POP_VALUE();
			a = POP_VALUE();
			value_boolean_set(&t1,
			    !value_equal(a, b)
			);
			PUSH_VALUE(&t1);
			VM_NEXT()

		/*** ARITHMETIC OPERATORS ***/

		/*
		 % ADD_INT : i i -> i
		 * Pop two integers, and push their sum as an integer.
		 */
		VM_OPLAB(INSTR_ADD_INT)
			b = POP_VALUE();
			a = POP_VALUE();
			value_integer_set(&t1,
			    value_get_integer(a) + value_get_integer(b)
			);
			PUSH_VALUE(&t1);
			VM_NEXT()

		/*
		 % MUL_INT : i i -> i
		 * Pop two integers, and push their product as an integer.
		 */
		VM_OPLAB(INSTR_MUL_INT)
			b = POP_VALUE();
			a = POP_VALUE();
			value_integer_set(&t1,
			    value_get_integer(a) * value_get_integer(b)
			);
			PUSH_VALUE(&t1);
			VM_NEXT()

		/*
		 % SUB_INT : i i -> i
		 * Pop two integers, and push their difference as an integer.
		 * The difference is the second popped minus the first.
		 */
		VM_OPLAB(INSTR_SUB_INT)
			b = POP_VALUE();
			a = POP_VALUE();
			value_integer_set(&t1,
			    value_get_integer(a) - value_get_integer(b)
			);
			PUSH_VALUE(&t1);
			VM_NEXT()

		/*
		 % DIV_INT : i i -> i
		 * Pop two integers, and push their quotient as an integer.
		 * The quotient is the second popped divided by the first.
		 */
		VM_OPLAB(INSTR_DIV_INT)
			b = POP_VALUE();
			a = POP_VALUE();
			value_integer_set(&t1,
			    value_get_integer(a) / value_get_integer(b)
			);
			PUSH_VALUE(&t1);
			VM_NEXT()

		/*
		 % MOD_INT : i i -> i
		 * Pop two integers, and push their remainder as an integer.
		 * The remainder is what is left over after dividing the
		 * second value popped by the first value popped.
		 */
		VM_OPLAB(INSTR_MOD_INT)
			b = POP_VALUE();
			a = POP_VALUE();
			value_integer_set(&t1,
			    value_get_integer(a) % value_get_integer(b)
			);
			PUSH_VALUE(&t1);
			VM_NEXT()

		/*** CONTROL FLOW INSTRUCTIONS ***/

		/*
		 % GOTO a : ->
		 * Unilaterally transfer control to a different
		 * code address.  This does not change the
		 * stack or activation record in any way.
		 */
		VM_OPLAB(INSTR_GOTO)
			pc++;
			pc = IMM_ADDR();
			pc--;
			VM_NEXT()

		/*
		 % FUN a : i -> f
		 * Create a new "functional value" (initial AR) from
		* the code address given by the operand, the size of
		* the activation record that will be required when it is
		 * invoked (given on the stack,) and the current
		 * activation record, (considered to be this fun's
		 * "enclosing" AR.)  Push this new fun onto the stack.
		 */
		VM_OPLAB(INSTR_FUN)
			pc++;
			a = POP_VALUE();
			value_ar_new(&t1, value_get_integer(a),
				     &VNULL, /* not set until called */
				     &ar, IMM_ADDR());
			PUSH_VALUE(&t1);
			VM_NEXT()

		/*
		 % NEW_AR i : ->
		 * Create a new activation record and use it for our
		 * execution context (stack etc.)
		 *
		 * This is generally only needed in the global
		 * scope, since INSTR_CALL takes care of creating
		 * AR's for itself.
		 *
		 * This implementation uses the 't1' value register
		 * temporarily to store the new AR.
		 */
		VM_OPLAB(INSTR_NEW_AR)
			pc++;
			value_ar_new(&ar, IMM_INT(), &ar, &VNULL, pc);
			VM_NEXT()

		/*
		 % CALL i : X v ->
		 * Pop a function-value from the stack and call
		 * it, passing a number of arguments given by
		 * the immediate integer.
		 */
		VM_OPLAB(INSTR_CALL)
			/*
			 * XXX should assert that it's an AR here.
			 */
			v = POP_VALUE();
			assert(value_is_tuple(v));

			pc++;		/* point at operand */

			/*
			 * Save the current program position in
			 * the current activation record.
			 */
			value_tuple_store_integer(&ar, AR_PC, pc + 1);

			value_tuple_store(v, AR_CALLER, &ar);

			/*
			 * Pass parameters to the new AR.
			 */
			XFER_VALUES(&ar, v, IMM_INT());

			/*
			 * Set the activation record and program
			 * counter up as our own.
			 */
			value_copy(&ar, v);
			pc = value_tuple_fetch_integer(&ar, AR_PC);
			pc--;
			VM_NEXT()

		/*
		 % RESUME i : X v ->
		 * Pop an AR from the stack and resume
		 * it, passing a number of arguments given by
		 * the immediate integer.
		 */
		VM_OPLAB(INSTR_RESUME)
			/*
			 * XXX should assert that it's right kind of tuple here.
			 */
			v = POP_VALUE();

			pc++;		/* point at operand */

			/*
			 * Save the current program position in
			 * the current activation record.
			 */
			value_tuple_store_integer(&ar, AR_PC, pc + 1);

			/*
			 * Use the existing AR for this fun.
			 */

			/*
			 * Pass parameters to the new AR.
			 * XXX note we should deal with
			 * resumes more cleanly.
			 */
			XFER_VALUES(&ar, v, IMM_INT());

			value_copy(&ar, v);
			pc = value_tuple_fetch_integer(v, AR_PC);
			pc--;
			VM_NEXT()

		/*
		 % YIELD i : X ->
		 * Transfer a number of values, given by the
		 * immediate integer, back up to the caller.
		 */
		VM_OPLAB(INSTR_YIELD)
			pc++;
			XFER_VALUES(&ar, value_tuple_fetch(&ar, AR_CALLER),
			    IMM_INT());
			VM_NEXT()

		/*
		 % RET : ->
		 * Transfer control back to the caller.
		 */
		VM_OPLAB(INSTR_RET)
			value_tuple_store_integer(&ar, AR_PC, pc + 1);  /* save pc in our ar */
			value_copy(&ar, value_tuple_fetch(&ar, AR_CALLER));  /* switch ar to caller */
			pc = value_tuple_fetch_integer(&ar, AR_PC);  /* move pc to caller */
			pc--;		  /* adjust for advance */
			VM_NEXT()

		/*
		 % REST : ->
		 * Yield to other processes in the system
		 */
		VM_OPLAB(INSTR_REST)
			VM_STOP()

		/*
		 % HALT : ->
		 * Stop this virtual machine.
		 */
		VM_OPLAB(INSTR_HALT)
			self->done = 1;
			VM_STOP()

		/*** CONDITIONAL CONTROL FLOW INSTRUCTIONS ***/

		/*
		 % JEQ a : v v ->
		 * Pop two values from the stack, and if they are
		 * equal, branch to a new location in the program
		 * given by the immediate address.
		 */
		VM_OPLAB(INSTR_JEQ)
			b = POP_VALUE();
			a = POP_VALUE();
			pc++;
			if (value_equal(a, b)) {
				pc = IMM_ADDR();
				pc--;
			}
			VM_NEXT()

		/*
		 % JNE a : v v ->
		 * Pop two values from the stack, and if they are not
		 * equal, branch to a new location in the program
		 * given by the immediate address.
		 */
		VM_OPLAB(INSTR_JNE)
			b = POP_VALUE();
			a = POP_VALUE();
			pc++;
			if (!value_equal(a, b)) {
				pc = IMM_ADDR();
				pc--;
			}
			VM_NEXT()

		/*
		 % JLT a : v v ->
		 * Pop two values from the stack, and if the second is
		 * less than the first, branch to a new location in the
		 * program given by the immediate address.
		 */
		VM_OPLAB(INSTR_JLT)
			b = POP_VALUE();
			a = POP_VALUE();
			pc++;
			if (value_compare(a, b) == CMP_LT) {
				pc = IMM_ADDR();
				pc--;
			}
			VM_NEXT()

		/*
		 % JLE a : v v ->
		 * Pop two values from the stack, and if the second is
		 * less than or equal to the first, branch to a new
		 * location in the program given by the immediate address.
		 */
		VM_OPLAB(INSTR_JLE)
			b = POP_VALUE();
			a = POP_VALUE();
			pc++;
			if (value_compare(a, b) != CMP_GT) { /* XXX */
				pc = IMM_ADDR();
				pc--;
			}
			VM_NEXT()

		/*
		 % JGT a : v v ->
		 * Pop two values from the stack, and if the second is
		 * greater than the first, branch to a new location in
		 * the program given by the immediate address.
		 */
		VM_OPLAB(INSTR_JGT)
			b = POP_VALUE();
			a = POP_VALUE();
			pc++;
			if (value_compare(a, b) == CMP_GT) {
				pc = IMM_ADDR();
				pc--;
			}
			VM_NEXT()

		/*
		 % JGE a : v v ->
		 * Pop two values from the stack, and if the second is
		 * greater than or equal to the first, branch to a new
		 * location in the program given by the immediate address.
		 */
		VM_OPLAB(INSTR_JGE)
			b = POP_VALUE();
			a = POP_VALUE();
			pc++;
			if (value_compare(a, b) != CMP_LT) { /* XXX */
				pc = IMM_ADDR();
				pc--;
			}
			VM_NEXT()

		/*** PROCESSES ***/

		/*
		 % OPEN : n n -> p
		 * Pop a name and a mode off the stack, open a
		 * file process for that name, and push it onto the
		 * stack.
		 */
		VM_OPLAB(INSTR_OPEN)
		    {
			struct process *p;

			b = POP_VALUE();	/* mode */
			a = POP_VALUE();	/* name */
			p = file_open(value_symbol_get_token(a), value_symbol_get_token(b));
			value_process_set(&t1, p);
			PUSH_VALUE(&t1);
		    }
			VM_NEXT()

		/*
		 % STDOUT : -> p
		 * Push a stream process representing the "standard" output
		 * onto the stack.  Note that this is not a good
		 * interface, and should be replaced asap, possibly
		 * by OPEN "*stdout" or something.
		 */
		VM_OPLAB(INSTR_STDOUT)
		    {
			struct process *p = file_open("*stdout", "w");

			value_process_set(&t1, p);
			PUSH_VALUE(&t1);
		    }
			VM_NEXT()

		/*
		 % CLOSE : p ->
		 * Pop a stream process off the stack and close it.
		 */
		VM_OPLAB(INSTR_CLOSE)
			a = POP_VALUE();
			stream_close(self, value_get_process(a));
			VM_NEXT()

		/*
		 % SPAWN a : -> p
		 * Create a new VM process based on the current
		 * process and start it in the scheduler.  The new
		 * process will share the code from this VM and
		 * will begin executing at the given address, but
		 * will not have any ARs of its own, nor will it have
		 * access to this process's ARs.
		 */
		VM_OPLAB(INSTR_SPAWN)
		    {
			struct process *spawned;

			value_vm_new(&t1, value_tuple_fetch(vm, VM_CODE));
			value_tuple_store(&t1, VM_AR, &VNULL);
			value_tuple_store(&t1, VM_IS_DIRECT, value_tuple_fetch(vm, VM_IS_DIRECT));
			pc++;
			value_tuple_store_integer(&t1, VM_PC, IMM_ADDR());

			spawned = vmproc_new(&t1);
			/* schedule!! */
			spawned->next = self->next;
			self->next = spawned;

			value_process_set(&t1, spawned);
			PUSH_VALUE(&t1);
		    }
			VM_NEXT()

		/*** INTER-PROCESS COMMUNICATION ***/

		/*
		 % WRITE : v s ->
		 * Pop a stream and a value from the stack
		 * and write the value in a 'raw' manner into
		 * the stream.  The value must be a tuple of
		 * small integers, which are interpreted as bytes.
		 * TODO: could also be a symbol, eh?
		 */
		VM_OPLAB(INSTR_WRITE)
		    {
			unsigned int i;
			unsigned char c;

			a = POP_VALUE();
			v = POP_VALUE();

			/*
			 * XXX this could certainly be optimized
			 * to use scatter/gather I/O and such
			 */
			for (i = 0; i < value_tuple_get_size(v); i++) {
				c = (unsigned char)value_get_integer(
				    value_tuple_fetch(v, i)
				);
				stream_write(self, value_get_process(a), &c, 1);
			}
		    }
			VM_NEXT()

		/*
		 % SEND : v p ->
		 * Pop a process and a value from the stack
		 * and send the value to the process.  The value
		 * will be packaged in such a way that a Kosheri
		 * process on the other end will be able to easily
		 * unpackage it to retrieve an exact copy of the
		 * original value.
		 */
		VM_OPLAB(INSTR_SEND)
			a = POP_VALUE();
			v = POP_VALUE();
			
			value_save(value_get_process(a), v);
			VM_NEXT()

		/*
		 % PORTRAY : v s ->
		 * Pop a stream and a value from the stack
		 * and render the value in a human-readable
		 * way, into the stream.
		 */
		VM_OPLAB(INSTR_PORTRAY)
			a = POP_VALUE();
			v = POP_VALUE();
			value_portray(value_get_process(a), v);
			VM_NEXT()

		/*** ADMINISTRATIVE ***/

		/*
		 % NOP : ->
		 * Explicitly do nothing.  Used for padding.
		 */
		VM_OPLAB(INSTR_NOP)
			VM_NEXT()

		/*
		 % EOF : ->
		 * Used to indicate the end of the VM bytecode array
		 * in certain contexts.  Should never be executed.
		 */
		VM_OPLAB(INSTR_EOF)
			assert(IMM_INT() != INSTR_EOF);
			VM_NEXT()

		VM_END_DISPATCH()
	}

	value_tuple_store(vm, VM_AR, &ar);
	value_tuple_store_integer(vm, VM_PC, pc);
}
