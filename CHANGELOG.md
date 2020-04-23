# Changelog
This log is sorted from oldest to newest, oldest first.

## Chapter 14
### 14.2 Getting Started
Builds clean using `tcc -o bin/clox src/*.c`.  I'm sure this'll fail later.

### 14.3 Chunks of Instructions
Bytecode chunks and dynamic array resizing.  Compiles and "runs."

### 14.4 Disassembling Chunks
Create a fake chunk and then disassemble it to stdout.

Started using `clangd` with VSCode for code completion.  For some reason it's
decided that all the .c files are actually C++, which is kinda weird.  There
does not seem to be any sort of option to deal with this problem, so now my files
are full of errors that aren't actually errors.

### 14.5 Constants
Added growable array for constant values to be tracked.

Also futzed around with the definitions of NULL and size_t to shut up `clangd`.

### 14.6 Line Information
Add tracking of line numbers through the bytecode.

## Chapter 15
Initial implementation through 15.3.  It turns out that merely installing
`clangd` doesn't actually install any of the header files it requires to run
correctly.  That's really fucking dumb.  Anyway, cleaned up some, but not all,
of the mess I added to define types that are clearly present.

I've altered the DEBUG_TRACE_EXECUTION output to indicate that the stack is
being printed, and that the stack can be empty.

Compiling is now being done with `tcc -Wall -o bin/clox src/*.c`.

### 15.3.1 Binary operators

An actual running program!  Needs verification.

## Chapter 16
Running with no errors.

## Chapter 17
Running with no errors.  Pretty amazing, actually.

## Chapter 18
Running with no errors.  I continue to be amazed that it always works the first time.

## Chapter 19
Again running with no errors.  It echoed "Hello, World!" from a concat.

## Chapter 20
Still running with no errors.  It validated that `("f" + "o" + "o") == "foo"`.
