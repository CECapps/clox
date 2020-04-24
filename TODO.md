# TODO
 - REPL
   - switch to readline/libedit/whatevs
   - add "exit"
 - Core functions that can write to stdout & stderr
 - Core functions that can read from stdin
 - Strings
   - Interpolation of select backslash escape sequences.
 - Chapter 18
   - What's with the dot syntax in value.h?  How does that get interpreted?
 - Language server
   - What is needed to build a language server for Lox?
 - Chapter 22, Section 4, "Using Locals"
   - In compiler.c identifiersEqual(), the book laments that variable names
     don't have hashes.  This sounds like a good idea to me.
   - In compiler.c endScope(), the book laments the number of OP_POPs that are
     emitted and suggests an OP_POPN instruction to perform multiple pops.
     (The dispatch of opcodes is said to be the most expensive operation.  Check
     that this is true...)
