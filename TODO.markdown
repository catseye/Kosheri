TODO
====

Correctness
-----------

* RECV instruction.

* Investigate why seemingly big-enough ARs are actually not big enough.

* Immutable values.  This will help solve the problems of using
  values as keys in a dictionary, and the problem of (not) sharing
  data between processes.

* (BIG) Stack type checker in assembler.

* (BIG) Fix file processes to really be concurrent, esp. in read.
  Use `select` (maybe export fd's to the scheduler.)

* (BIG) Error handling.  Instead of just `assert()` (which isn't even there
  when you define `NDEBUG`), throw an exception if the unexpected
  happens.  Or, send a message to the process which created this
  process.  (This will require `self` be accessible throughout the code
  somehow.)

Testing
-------

* Falderal tests for all variants of conditionals.

* Falderal test for closures.

* Falderal test for sending and receiving messages.

* Falderal tests for dictionaries.

* Falderal test for portraying cyclic values.

* Falderal test for saving cycling values.

Accessibility
-------------

* Document C APIs (header files).

* enum success { SUCCESS = 1; FAILURE = 0; } and return this instead of int.

* Portray values from stream_render: %v or similar.

* Figure out way to debug messages received by file process
  (can't write to file process, or you'll get an infinite loop.)

Portability
-----------

* Measure sizeof struct value et al in buildinfo.  Spit out defines.
  Think about how 64-bit systems will handle all this.

Footprint
---------

* "small immediates" embedded in appropriate VM instructions.

* Go back to having "functional values", but basically say
  that these are ARs with zero space allocated for arguments / locals.
  Then have a VM instruction to create an AR from a functional value.
  (Maybe also have a VM instruction to create a full AR instead of a
  functional value, from a label, for when you want to call it one-off.)

Performance
-----------

* Allow symbols (/strings) to be interned.  There is of course a
  tradeoff here, so "allow" rather than "require".
  Maybe have VALUE_TAG being "small strings" (fit in a struct value,
  so, like, 7 characters).  Then many problems go away...?

* Option to create a (non-interned) symbol from a const string in
  a way that does not copy the const string.

* More efficient access of free variables.  Split each AR into two sections: bound variables
  and free variables.  The bound variables are stored in the AR itself.  Free variables are
  stored in some other AR; pointers to them are stored in this AR.  Then accessing a bound
  variable is only a single indirection.  Tradeoff is that more work needs to be done when
  creating a functional value.

* In ARs, store top-of-stack pointer as a machine pointer,
  not a tuple index.
