/*
 * disasm.c
 * Disassembler for virtual machine.
 */

#include "lib.h"
#include "cmdline.h"

#include "value.h"
#include "chain.h"

#include "file.h"
#include "stream.h"
#include "render.h"

#include "report.h"
#include "instrtab.h"
#include "load.h"
#include "portray.h"

/* Routines */

static void
disassemble(struct reporter *r, struct process *p, struct value *code)
{
	struct opcode_entry *oe;
	int count, pc, opcode;
	struct value *val;
	struct value t;
	struct chain *back, *front;

	value_integer_set(&t, 0);

	/* first pass: find label destination */

	pc = 0;
	back = front = add_to_chain(NULL, &t);
	for (;;) {
		val = value_tuple_fetch(code, pc);
		if (value_is_integer(val)) {
			opcode = value_get_integer(val);
			if (opcode < 0 || opcode > INSTR_EOF) {
				report(r, REPORT_ERROR, "Opcode not in range 0..%d", INSTR_EOF);
				pc++;
			} else if (opcode == INSTR_EOF) {
				break;
			} else {
				oe = &opcode_table[opcode];
				count = oe->arity;
				pc++;
				val = value_tuple_fetch(code, pc);
				while (count > 0) {
					if (oe->optype == OPTYPE_ADDR)
						back = add_to_chain(back, val);
					pc++;
					val = value_tuple_fetch(code, pc);
					count--;
				}
			}
		} else {
			report(r, REPORT_ERROR, "Opcode is not an integer");
			pc++;
		}
	}

	if (reporter_has_errors(r))
		return;

	/* second pass */
	pc = 0;
	for (;;) {
		val = value_tuple_fetch(code, pc);
		opcode = value_get_integer(val);
		if (opcode == INSTR_EOF)
			break;

		value_integer_set(&t, pc);
		if (search_chain(front, &t) != NULL) {
			process_render(p, ":L%d\n", pc);
		}

		oe = &opcode_table[opcode];
		process_render(p, "%s ", oe->token);
		count = oe->arity;

		pc++;
		val = value_tuple_fetch(code, pc);

		if (count == -1) {
			report(r, REPORT_WARNING, "Unrecognized opcode");
			process_render(p, "??? ");
		} else {
			while (count > 0) {
				if (oe->optype == OPTYPE_ADDR) {
					process_render(p, ":L%d ",
					    value_get_integer(val)
					);
				} else {
					process_render(p, "#");
					value_portray(p, val);
				}
				pc++;
				val = value_tuple_fetch(code, pc);
				count--;
			}
		}

		process_render(p, "\n");
	}

	free_chain(front);
}

static void
disassemble_main(struct value *args, struct value *result)
{
        struct reporter *r;
	struct process *p;
	struct value code;	/* virtual machine code we will dump */
	struct value *asmfile, *vmfile;
        struct value asmfile_sym, vmfile_sym;

        value_symbol_new(&asmfile_sym, "asmfile", 7);
        value_symbol_new(&vmfile_sym, "vmfile", 6);

  	asmfile = value_dict_fetch(args, &asmfile_sym);
	vmfile = value_dict_fetch(args, &vmfile_sym);

	r = reporter_new("Disassembly", NULL, 1);

	/*
	 * Load.
	 */
	p = file_open(value_symbol_get_token(vmfile), "r");
	value_load(&code, p);
	stream_close(NULL, p);

	p = file_open(value_symbol_get_token(asmfile), "w");
	disassemble(r, p, &code);
	stream_close(NULL, p);

	value_integer_set(result, reporter_has_errors(r) ? 1 : 0);
	reporter_free(r);
}

MAIN(disassemble_main)
