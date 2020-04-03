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
