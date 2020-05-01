# TODO
 - REPL
   - switch to readline/libedit/whatevs
 - Core functions that can write to stdout & stderr
 - Core functions that can read from stdin
 - Chapter 18
   - What's with the dot syntax in value.h?  How does that get interpreted?
 - Language server
   - What is needed to build a language server for Lox?
 - Chapter 22, Section 4, "Using Locals"
   - In compiler.c endScope(), the book laments the number of OP_POPs that are
     emitted and suggests an OP_POPN instruction to perform multiple pops.
     (The dispatch of opcodes is said to be the most expensive operation.  Check
     that this is true...)

# Challenges

## Implementation Changes

### Immediate Priority
 - C20c1: Add hashtable support for numbers, booleans, and nil.
   - ... and functions, and classes, and ...
 - C24c2: Add a mechanism for native functions to provide an error message.

### Soon Priority
 - C15c3: dynamically growable stack.  How does this interact with the function
   call frame mechanism?
 - C21c2: Investigate Radix Trees and replace the global variable mechanism with one.
 - C22c1: Investigate replacing local variable lookups with a hashtable or a
   Radix Tree, in the same way as globals.
   - In compiler.c identifiersEqual(), the book laments that variable names
     don't have hashes.  This sounds like a good idea to me.

### Later Priority
 - C14c1: optimize how line and position encoding works.
 - C14c2: Define an operation to get a value stored as a long number.  This may
   be happening in the book already.
 - C22c4: Add support for more than 255 local variables.  After C15c3 and C14c2.

## Language Features

### Immediate Priority
 - C20c1: After adding support for all `Obj`s as keys in the internal hashtable
   mechanism (Impl->Immediate), expose hashtables to the language through native
   functions:
     - hashtable = ht_create();
     - mixed = ht_get(hashtable, key)
     - mixed = ht_set(hashtable, key, value)
     - bool = ht_update(hashtable, key, value)
     - bool = ht_unset(hashtable, key)
     - bool = ht_has(hashtable, key)
     - number = ht_count_keys(hashtable)
     - clear(hashtable)

### Soon Priority
 - C17c3: The `?:` operator.
 - C19c3: Add a separate operator for concatenation. `+` should be numbers only.
 - C22c3: Add a mechanism to create variables that can not be reassigned.  I
   shall call these `unvar`, because they are variables that are not varaible.
   Difficulty level: the challenge wants this to be a compile time error...

### Later Priority
 - C16c1: Interpolation of expressions inside double-quoted strings.
 - C23c1: Add a `switch` statement.  After C17c3.
 - C23c3
   - Add `until` as a counterpart to `while`,
   - Add `unless` as a counterpart to `if`.
   - Add `loop(number)` as a counterpart to `for`.
 - C23c2: Add a `continue` statement for loops.


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
 - ~~Interpolation of certain backslash sequences~~ **DONE** in all double-quoted strings.
 - ~~Length in 8-bit bytes~~ **DONE**, use `string_length()`, returns false with bogus input
 - ~~Extracting a substring at a starting index for a given length~~ **DONE**, use `string_substring(string, index, length?)`
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

## Unrequired Functions
I'm adding these because I feel these will be handy.

 - System
   - Get environment variable
 - Variable Meta
   - Variable has a value (false only for empty string, number zero, nil, empty hashtable)
   - Variable is string
   - Variable is number
   - Variable is boolean
   - Variable is NaN
   - Variable is Infinity (positive or negative)
   - Get variable hash value as hex string
 - Number
   - Absolute value
   - Remainder from integer division
   - Minimum value in a list
   - Maximum value in a list
   - Floor
   - Ceiling
   - Clamp between two values
   - Round to precision, positive or negative
   - Convert to string, with precision via rounding
   - Convert to hex string, integer value only via rounding
   - Create from string
 - String
   - Reverse
   - Character to byte integer
   - Byte integer to character
   - Location of substring, with starting index
   - Location of substring, starting on the right, with starting index
   - Contains substring (location != nil) (*userland?*)
   - Starts with substring (location == 0) (*userland?*)
   - Ends with substring (location == strlen - substr len) (*userland?*)
     - All of these substring functions can probably share the same util function
   - Trim substring=whitespace
   - Trim substring=whitespace from right only
   - Trim substring=whitespace from left only
   - Left pad to given length
   - Right pad to given length
   - Matches C stdlib regex
   - Escape chars to fit in a double-quoted string (\n, \r, \t, \x##, etc)
   - Splice(original_string, new_string, insert_index, maximum_length?)
   - Replace all instances of substring in string with a second substring
