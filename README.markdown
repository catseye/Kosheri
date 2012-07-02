Kosheri
=======

Kosheri is a stack-based virtual machine.  Here's what it offers:

* Garbage-collection, tagged tuples, dictionaries, function values,
  coroutines, lightweight processes, textual and binary serialization,
  and an assembler and disassembler.  See "Features", below, for
  more information.
* A really small code footprint.  On Ubuntu 10.04 LTS, a minimal VM
  executor compiles to an executable less than 16K in size.
* A clean, almost pedantic implementation.  See "Implementation", below.
* Very few build dependencies.  See "Build requirements", below.
* An extremely orthogonal architecture.  See "Architecture", below.
* A reasonably efficient implementation.  See "Performance", below.

Features
--------

* Ability to manipulate values of boolean, 32-bit integer,
  process reference ("pid"), immutable string ("symbol"), and
  tagged tuple type.
* Support for manipulating certain tagged tuple types:
  dictionaries, function values, and activation records.
* Support for closures and coroutines via appropriate use of
  activation records.  (ARs retain some state after being
  called; if this is not cleared, they can be continued.)
* A simple, mark-and-sweep garbage collector (for tuples and
  symbols; everything else lives on the stack.)
* Concurrent operation.  Each lightweight process can be a
  VM process or a native process.  Native processes are used to
  implement interfaces to the rest of the world.  Multitasking
  is pre-emptive for VM processes, and co-operative for native
  processes.  Concurrency is implemented in the VM; system threads
  and processes are not used.  Interprocess communication is done
  with Erlang-style messaging to processes' mailboxes.

Implementation
--------------

The code is generally written in BSD style(9).  It compiles under
`-ansi -pedantic` with almost all `gcc` warnings enabled and treated as
errors.  The core interfaces are `const`ified.  `assert`s are
plentiful.  I've tried to limit the amount of typecasting used in the
code, but of course there are some places where it is necessary.

Functions are often written in a straightforward, almost pedantic
fashion; there are of course some exceptions here too, like the
direct threading support.

Build requirements
------------------

Only the following tools are required to build the VM and its tools:

* an ANSI C compiler (default is `gcc`) and linker (`ranlib`, `ar`)
* GNU Make

I have vague plans to remove even the dependency on GNU Make (the
code is so small that rebuilding the whole thing is not a huge burden,
so it could be done with a shell script or a custom build tool.)

Even `libc` is not a strict requirement for building the VM; a
few functions from `libc` that the code uses are implemented
independently in the code.  The `standalone` target builds the
system with `-nostdlib`.  However, while it builds it currently
does not link, as it requires some memory allocator to link to,
and it doesn't have one... yet.

Architecture
------------

From inside the virtual machine, it's a truism that "everything's
a value, and every value that's not an atomic value is a tagged tuple."
The latter is true even for VM code and activation records and
dictionaries.

Moreover, this truism largely holds in the C code as well.  In many
places in the assembler and elsewhere, we don't just use C structs
to hold our data, we use `struct value`s.

Every value has defined external representations, in human-readable and
compact binary forms, which can be generated and parsed.

The rest of this section is kind of scattered...

Some more notes on how the runtime is written in C...

Every C function in the Kosheri runtime is run in some environment.

We try to assume as little about the surrounding environment (e.g., OS)
as possible, and to present the world to these functions in as Kosheri-
like a way as possible.

Maybe not every function, but lots of functions.  The ones that really
matter.  Top-level functions especially -- (the ones that would be
command-line utilities in a Unix system.)

Every such function is passed a runtime environment.  This runtime
environment consist of:

-- arguments that were passed to the function in the form of a Kosheri
dictionary (value_dict).  The function may also have a data structure
associated with it (the "argdecl") that facilitates checking these
arguments for syntactic correctness and automatically returning an error
if they don't meet those checks.  (this part's not done yet)

-- environment variables that were made available to the function.
(This is possibly not OK.  Environment variables have a way of hanging
around that arguments don't.  Some kind of adapter is called for that
translates (relevant) env vars to arguments first.)

-- streams available to the function, in the form of Kosheri processes.
The function should not assume the presence of a particular stream, or
at least, a function which does assume the presence of a particular
stream should not be assumed to always work.  The function should also
not assume that the streams that are made available to it behave in a
particular way (for example, that a stream is directed to a vt100
terminal.)  Instead it should look for a stream that abstractly presents
the behaviour that it wants.

Performance
-----------

Kosheri was designed to be reasonably efficient, for a virtual machine
without a JIT compiler.

The design decisions for performance are kind of all over the place,
sometimes being extremely optimized, sometimes sacrificing pure
performance for flexibility.

* "Direct threading" is possible with C compilers that support it,
  such as `gcc`.  This basically optimizes the main instruction-
  selection `switch` into a computed `goto`.

* The compiled VM is small, really small.  This means it can usually
  fit entirely in the cache, and stay there.  This can sometimes result
  in a significant performance benefit.

On the other hand...

* The orthogonality of "everything is a tuple" extends far and wide.
  All values are 8 bytes, including VM instructions.  VM code size
  could be reduced by packing 2 or 4 instructions into that 8 bytes,
  but we don't do that yet.

* Input and output are modelled as processes, which means there is
  some small overhead (to pack and unpack a message) added to I/O;
  but I/O is I/O-bound anyway, so this is probably not something to
  worry about.


Tour of the distribution
------------------------

    bin/

Where compiled binaries go.

    eg/

Example assembly language files.

    lib/

Where compiled libraries (static and dynamic) go.

    src/

Source code for the virtual machine and tools.

    src/build/

Source code for tools used during build-time, and not thereafter.

    tests/

Tests.


Tour of the source
------------------

    assemble.c

Main program for the assembler.

    chain.c
    chain.h

Utilities to accumulate a linked list of values, then turn them
into a tuple when accumulation has finished.

    cmdline.c
    cmdline.h

Harness which translates command-line arguments to dictionaries,
and otherwise provides facilities so that programs can be written
in a "Kosheri view" of the outside world.

    disasm.c

Main program for the disassembler.

    discern.c
    discern.h

Routines to parse human-readable representation of tuples into
the internal format.

    file.c
    file.h

Native implementation of processes supporting the stream-like interface
and which are backed with C's stdio.

    freeze.c

Main program for the human-readable-value-to-compact-binary
serializer.

    gen.c
    gen.h

Routines for generating VM instructions (used by assembler, but
would also be useful for compilers targeting this VM.)

    geninstr.c

A build tool which generates instrtab.c and instrenum.h from
vm.c.

    instrtab.h

Header file for the generated instrtab.c.

    lib.c
    lib.h

Clean-room re-implementations of the few libc-supplied functions
which the VM uses.

    load.c
    load.h

Routines to parse (unserialize) the compact binary representation
of values.

    portray.c
    portray.h

Routines to format values into a human-readable representation.

    process.c
    process.h

Routines for communicating and switching between co-operative
lightweight concurrent processes.

    render.c
    render.h

Routines for rendering terms to file-like processes.

    report.c
    report.h

Routines for reporting errors, successes, etc. for the assembler,
disassembler, freezer and thawer.

    run.c

Main program for the VM runner.  Takes a VM program in the compact
binary representation and executes it.

    save.c
    save.h

Routines to generate (serialize) the compact binary representation
of values.

    scan.c
    scan.h

Lexical scanner, used by the assembler, and by the discern routines.
Fairly general, so could also be used to parse a language being
compiled to the VM.

    stream.c
    stream.h

Routines to communicate with processes which support the stream-like
interface; you can read from them, write to them, check for eof, and
close them.

    thaw.c

Main program for the compact-binary-to-human-readable-value
unserializer.

    value.c
    value.h

Data structure (and associated functions) which internally
represents values, including atomic values (booleans, integers,
process id's, direct-threaded opcodes) and structured values
(tagged tuples and symbols.)

    vm.c
    vm.h

The virtual machine implementation itself.

    vmproc.c
    vmproc.h

The implementation of lightweight processes which uses the virtual machine
to execute the process.
