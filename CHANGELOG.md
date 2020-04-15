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

### Challenge 2

I am heavily in favor of more instructions instead of fewer instructions.  For
this specific project, I do not care about speed or optimization.  I would not
replace the numeric negate operator or the subtraction operator.  They serve
different purposes in different contexts.

## Chapter 16
Running with no errors.

### Challenge 1
> What token types would you define to implement a scanner for string interpolation?

I'd probably only need to add one token, for `${`, but I'm not sure I'd go about
doing too much processing during the scan.  I expect the most sane way to go
about doing string interpolation would be to rewrite the given string into a set
of concats.  Given the demo string:

> `"${drink} will be ready in ${steep + cool} minutes."`

I'd want to add (virtual?) parens for precedence and interpret it as:

> `((drink) + " will be ready in " + (steep + cool) + " minutes.")`

I'm not sure where in the processing this transformation would have to occur.  I
basically want to allow any expression within the `${...}` block.

> What tokens would you emit for
>
> `"Nested ${"interpolation?! Are you ${"mad?!"}"}"`

Yikes.  I'd emit a syntax error, and probably delete the source code file out
of spite.

## Chapter 17
Running with no errors.  Pretty amazing, actually.

### Challenge 2 (Infix Operators)
> What about in C or another language of your choice?

The first two that come to mind are the pointer operators, * and &.  These are
also used for multiplication and bitwise-and respectively.
