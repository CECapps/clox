# TODO
 - REPL
   - switch to readline/libedit/whatevs
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

# loxlox requirements

## Required Compiler Operations
 - `include`
   - Needed for code generation as outlined in the book.  Could skip code gen...
   - Splice the specified file into the current string.
   - Done at compile time, not run time.  Expects a string, not an expression.

## Required I/O Operations
 - Reading the contents of a file into a string
 - Reading a line from stdin
 - ~~Printing to stdout without a newline~~ **DONE**, use `echo` instead of `print`.
 - Opening a file and writing to it one line at a time?
   - Needed for code generation as outlined in the book.  Could skip code gen...

## Required String Operations
 - Interpolation of certain backslash sequences
   - See book text for the exact expectations
   - Some other way of representing unprintable/control characters could also work
 - Length in 8-bit bytes
 - Extracting a substring at a starting index for a given length
 - Splitting a string along a given character
   - If I end up with userland Arrays, this'd be an Array method
 - Trimming a string

## Required Language Features
 - ~~Exiting, with a status code~~ **DONE**, use `exit`, with optional numeric exit code.
 - Enums
   - They're just constants, right? *Right?*
   - Could just use the same assign-once operation as defined in Chapter 22 Challenge 3
 - `?:`?
   - Not really needed, but there's a Challenge for it, so ...
 - A "does this variable exist" ... operator?
   - Hmm, `exists foo` vs `exists(foo)`?
 - A way to determine if a variable is a string
 - A way to determine if a variable is a number
 - A "is this variable an instance of this class" operator
   - Hmm, `foo instanceof ClassName`
   - Hmm, `foo instanceof "ClassName"`?
   - ... `exists foo and foo instanceof number`?  `isa number`?
   - Is there a difference in how `isa` would be understood by humans vs `instanceof`?
 - Varargs?
   - For `match` in Chapter 6
 - Optional function arguments
   - `fun foo(bar, baz?) { ... }` ?
   - `fun foo(bar, baz = null) { ... }` ?
      - Don't seem to need actual default values, so this might be overkill
 - Arrays
   - ... or objects that look like an Array
 - Hashes
   - ... or objects that look like a Hash

## Required OO Features
 - Exceptions
   - Userland throwing should be easy.  `fun:return :: try:throw`
   - `catch` is just an anonymous non-closure function with a default parameter?
   - Ugh, the environment code in chapter 8 uses `finally`.
 - Interfaces
   - Could just make a default impl for each method that throws an error.
 - Abstract classes?
   - Maybe?  Can simulate them by just adding a bogus initializer.
 - Nested classes?
   - Maybe?  Only used in the book to keep everything in one file, because Java
     doesn't let you define multiple classes per source file.
