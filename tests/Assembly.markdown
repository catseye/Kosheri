Kosheri Assembly
================

-> Functionality "Round-trip Kosheri Assembly" is implemented by shell command
-> "./assemble --asmfile %(test-file) --vmfile foo.kvm && ./disasm --vmfile foo.kvm --asmfile %(output-file)"

-> Tests for functionality "Round-trip Kosheri Assembly"

    | NEW_AR #10
    | GOTO :past_q
    | :q
    | 			; no need to reserve space for parameters
    | GETI #0		; local #0 = 1st parameter = a
    | STDOUT
    | PORTRAY
    | 
    | GETI #1		; local #1 = 2nd parameter = b
    | STDOUT
    | PORTRAY
    | 
    | GETI #0
    | GETI #1
    | ADD_INT
    | 
    | YIELD #1		; pass the result back to our caller
    | RET			; end of this function
    | 
    | :past_q
    | 
    | PUSH #1
    | 
    | PUSH #2
    | PUSH #3
    | PUSH #10		; this function will need 4 slots: 2 args, 2 stack
    | FUN :q		; push a closure for q onto the stack
    | CALL #2		; call it with two args
    | 
    | PUSH #10		; this function will need 4 slots: 2 args, 2 stack
    | FUN :q		; push a closure for q onto the stack
    | CALL #2		; call it with two args
    | 
    | STDOUT
    | PORTRAY
    | 
    | HALT
    = :L0
    = NEW_AR #10
    = GOTO :L20 
    = :L4
    = GETI #0
    = STDOUT 
    = PORTRAY 
    = GETI #1
    = STDOUT 
    = PORTRAY 
    = GETI #0
    = GETI #1
    = ADD_INT 
    = YIELD #1
    = RET 
    = :L20
    = PUSH #1
    = PUSH #2
    = PUSH #3
    = PUSH #10
    = FUN :L4 
    = CALL #2
    = PUSH #10
    = FUN :L4 
    = CALL #2
    = STDOUT 
    = PORTRAY 
    = HALT 
    = 

Unused labels aren't produced by the disassembler.

    | NEW_AR #5
    | :here
    | PUSH #123
    | STDOUT
    | PORTRAY
    | HALT
    = :L0
    = NEW_AR #5
    = PUSH #123
    = STDOUT 
    = PORTRAY 
    = HALT 
    = 

Redefining a label is an error.

    | NEW_AR #5
    | :here
    | PUSH #123
    | STDOUT
    | PORTRAY
    | :here
    | HALT
    ? line 6, column 6, token 'here'): Assembly Error: Label already defined.
    ? Assembly finished with 1 errors and 0 warnings

You can include literal terms of all kinds in an assembly file.

    | NEW_AR #5
    | PUSH #<tuple: THIS, IS, 1, TUPLE>
    | STDOUT
    | PORTRAY
    | HALT
    = :L0
    = NEW_AR #5
    = PUSH #<tuple: THIS, IS, 1, TUPLE>
    = STDOUT 
    = PORTRAY 
    = HALT 
    = 

-> Functionality "Run Kosheri Assembly" is implemented by shell command
-> "./assemble --asmfile %(test-file) --vmfile foo.kvm >/dev/null 2>&1 && ./run --vmfile foo.kvm"

-> Tests for functionality "Run Kosheri Assembly"

Hello, world-ing
----------------

You can include literal terms of all kinds in an assembly file.

    | NEW_AR #5
    | PUSH #<tuple: THIS, IS, 1, TUPLE>
    | STDOUT
    | PORTRAY
    | HALT
    = <tuple: THIS, IS, 1, TUPLE>

Basic Arithmetic
----------------

Add eight to two and output the answer.

    | NEW_AR #2
    | PUSH #8
    | PUSH #2
    | ADD_INT
    | STDOUT
    | PORTRAY
    | HALT
    = 10

Subtract three from twelve and output the answer.

    | NEW_AR #2
    | PUSH #12
    | PUSH #3
    | SUB_INT
    | STDOUT
    | PORTRAY
    | HALT
    = 9

Multiply 6 by 7.

    | NEW_AR #2
    | PUSH #6
    | PUSH #7
    | MUL_INT
    | STDOUT
    | PORTRAY
    | HALT
    = 42

Divide 40 by 5.

    | NEW_AR #2
    | PUSH #40
    | PUSH #5
    | DIV_INT
    | STDOUT
    | PORTRAY
    | HALT
    = 8

Basic Looping
-------------

Count down from 11 to 1.

    | NEW_AR #3
    | PUSH #11	; statically initialize our local
    | :label
    | GETI #0
    | STDOUT
    | PORTRAY
    | GETI #0
    | PUSH #1
    | SUB_INT
    | SETI #0
    | GETI #0
    | PUSH #0
    | JNE :label
    | HALT
    = 1110987654321

Function calling
----------------

Call a simple function.

	local q = function(a)
	    print a
	    return 5
	end
	print q(4)

    | NEW_AR #5
    | PUSH #0	; move stack pointer up for one local, q
    | GOTO :past_q
    | :q
    | 		; no need to reserve space for parameters
    | GETI #0	; local #0 = 1st parameter = a
    | STDOUT
    | PORTRAY
    | 
    | PUSH #5
    | YIELD #1	; pass the result back to our caller
    | RET		; end of this function
    | 
    | :past_q
    | PUSH #5	; this function will need 2 slots: 1 arg, 1 stack
    | FUN :q	; push a closure for q onto the stack
    | SETI #0	; set local #0 = q
    | 
    | PUSH #4	; put argument to pass on the stack
    | GETI #0	; put function (AR) to call (activate) on the stack
    | CALL #1	; call function, with one argument
    | 
    | STDOUT	; return value is on the stack.  print it
    | PORTRAY
    | 
    | HALT
    = 45

Make two calls to a function.

	local q = function(a, b)
	    print a
	    print b
	    return a + b
	end
	print q(1, q(2, 3))

    | NEW_AR #10
    | GOTO :past_q
    | :q
    | 			; no need to reserve space for parameters
    | GETI #0		; local #0 = 1st parameter = a
    | STDOUT
    | PORTRAY
    | 
    | GETI #1		; local #1 = 2nd parameter = b
    | STDOUT
    | PORTRAY
    | 
    | GETI #0
    | GETI #1
    | ADD_INT
    | 
    | YIELD #1		; pass the result back to our caller
    | RET			; end of this function
    | 
    | :past_q
    | 
    | PUSH #1
    | 
    | PUSH #2
    | PUSH #3
    | PUSH #10		; this function will need 4 slots: 2 args, 2 stack
    | FUN :q		; push a closure for q onto the stack
    | CALL #2		; call it with two args
    | 
    | PUSH #10		; this function will need 4 slots: 2 args, 2 stack
    | FUN :q		; push a closure for q onto the stack
    | CALL #2		; call it with two args
    | 
    | STDOUT
    | PORTRAY
    | 
    | HALT
    = 23156

Call an iterator.

	local q = function()
	    yield 1
	    yield 3
	    yield 5
	end
	print q()
	print q()
	print q()

    | NEW_AR #7
    | PUSH #0		; move stack pointer up for one local, q
    | GOTO :past_q
    | :q
    | PUSH #1
    | YIELD #1	; yield one value
    | RET
    | PUSH #3
    | YIELD #1	; yield another value
    | RET
    | PUSH #5
    | YIELD #1
    | RET
    | 
    | :past_q
    | PUSH #7		; this function will need 1 local
    | FUN :q		; push a closure for q onto the stack
    | SETI #0		; set local #0 = q
    | 
    | GETI #0		; call q three times; should get a different
    | CALL #0		; result each time.
    | STDOUT
    | PORTRAY
    | 
    | GETI #0
    | RESUME #0
    | STDOUT
    | PORTRAY
    | 
    | GETI #0
    | RESUME #0
    | STDOUT
    | PORTRAY
    | 
    | HALT
    = 135

Return a function from a function.

	local f = function(a)
	    print a
	    local g = function(b)
	        return b
	    end
	    return g
	end
	h = f(3)
	print h(9)

    | NEW_AR #7
    | PUSH #0
    | PUSH #0
    | GOTO :past_f
    | :f 
    | GETI #0
    | STDOUT
    | PORTRAY
    | 
    | GOTO :past_g
    | :g
    | GETI #0
    | YIELD #1
    | RET
    | 
    | :past_g
    | PUSH #7
    | FUN :g
    | YIELD #1
    | RET
    | 
    | :past_f
    | 
    | PUSH #7		; this function will need 1 local
    | FUN :f		; push a closure for f onto the stack
    | SETI #0
    | 
    | PUSH #3
    | GETI #0
    | CALL #1               ; now we'll have a fun on the stack
    | 
    | SETI #1
    | PUSH #9
    | GETI #1
    | CALL #1
    | 
    | STDOUT
    | PORTRAY
    | 
    | HALT
    = 39

Call a closure.

	local f = function(a)
	    local g = function(b)
	        return a + b
	    end
	    return g
	end
	h = f(3)
	print h(9)

    | HALT
    = 

Spawn a process!

The behaviour of this might rely on multithreading details...
like that we execute 100 cycles before switching.

    | NEW_AR #7
    | SPAWN :label
    | REST
    | PUSH #main
    | STDOUT
    | PORTRAY
    | HALT
    | :label
    | NEW_AR #7
    | PUSH #worker
    | STDOUT
    | PORTRAY
    | HALT
    = workermain
