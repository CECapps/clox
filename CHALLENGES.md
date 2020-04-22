# Challenges
This is a list of the challenges from Chapters 14-30 of Crafting Interpreters.
As I work my way through the book, I will be briefly evaluating each challenge
to determine how to approach it.  I'm grouping challenges into three general
categories, with three generally different default responses:

 - Challenges that require non-trivial research, with the usual goal of attaining
   deeper understanding.
   - I'm not planning on performing much research here.  These will usually be
     skipped.
 - Challenges to add/change features in the language.
   - These are almost always going to be deferred until the end, often because
     related work is being performed later on that will make the change easier.
 - Challenges that prompt for opinions.
   - Oh boy do I have *opinions!*

## Chapter 14, "Chunks of Bytecode"
### Challenge 1
> Devise an encoding that compresses the line information for a series of
> instructions on the same line.

Hard **defer**, possibly even skipping.  Because this information is only used
in error messages, I'm going to want to review it when it's time to make those
error messages prettier and more useful.

### Challenge 2
> Leave our existing one-byte `OP_CONSTANT` instruction alone, and define a
second  `OP_CONSTANT_LONG` instruction. It stores the operand as a 24-bit
number, which should be plenty.

**Deferring** because I vaguely remember seeing something similar to this happen
later in the book.

### Challenge 3a
> Find a couple of open source implementations of them (`malloc`) and explain
how they work. How do they keep track of which bytes are allocated and which are
free? What is required to allocate a block of memory? Free it? How do they make
that efficient? What do they do about fragmentation?

This is actually a pretty interesting one.  I remember long ago that Mozilla
switched to a different `malloc` implementation (it started with a "j"?) that
ended up making the entire application faster and use less memory overall.
I want to find out what it was, why it worked so well, and if that decision
is still present in today's Firefox codebase.

**Deferring** due to it being research.

### Challenge 3b
> Hardcore mode: Implement `reallocate()` without calling `realloc()`,
`malloc()`, or `free()`. You are allowed to call `malloc()` once, at the
beginning of the interpreter’s execution, to allocate a single big block of
memory which your `reallocate()` function has access to. It parcels out blobs
of memory from that single region, your own personal heap. It’s your job to
define how it does that.

Hard **decline.**  If I wanted to do low-level memory management, I'd be
working in assembly language on a low-memory system.

## Chapter 15, "A Virtual Machine"

### Challenge 1
> What bytecode instruction sequences would you generate for the following
expressions:

**Deferring** until operator precedence, because the generated opcodes wouldn't
make any coherent sense until then.

### Challenge 2
> Given the above, do you think it makes sense to have both (`OP_NEGATE` and
`OP_SUBTRACT`) instructions? Why or why not? Are there any other redundant
instructions you would consider including?

There is absolutely a case for negating the current value being a different op
than subtracting one value from another.  That being said, I disagree with using
the negate operation to create negative numbers instead of actually just parsing
the number into being negative to begin with.

I am heavily in favor of more instructions instead of fewer instructions.  For
this specific project, I do not care about speed or optimization.

### Challenge 3
> Our VM’s stack has a fixed size, and we don’t check if pushing a value
overflows it.  [...] Avoid that by dynamically growing the stack as needed.
What are the costs and benefits of doing so?

Good idea, but **deferred** with **medium** priority.  The main cost would be
a size check on `push` and the mechanism of creating a new stack, copying over
the contents, then freeing the old.  The main benefit would be allowing larger,
deeper, more complex programs, with more recursion etc.

## Chapter 16, "Scanning on Demand"

### Challenge 1
> What token types would you define to implement a scanner for string
interpolation? What sequence of tokens would you emit for the above string
literal? (`print "${drink} will be ready in ${steep + cool} minutes.";`)

**Deferred** with **high** priority.  I intend to support the following string
interpolations in double quoted strings:

 - `"\n"` => newline
 - `"\r"` => carriage return
 - `"\t"` => tab
 - `"\\"` => backslash
 - `"\""` => double quote
 - `"\x##"` => a single 8-bit byte with the value of ## in hex

Once I determine what's needed for proper Unicode (UTF-8) support, I expect to
add the `\u####` and `\u{...}` formats for Unicode characters.

All of these interpolations can be made at compile time, so no change in the
emitted tokens would be required.  Variable and expression interpolation is a
different beast.  I intend to support the following variable and expression
interpolations within quoted strings:

 - `"foo ${expression} bar"` => `'foo ' + (expression) + ' bar'`
 - `"foo {$variable} bar"` => `'foo ' + variable + ' bar'`

Expression syntax will have the parens added to force correct operator precdence.

I also intend to add support for single-quoted strings, which will have no
interpolation whatsoever, other than allowing `'\''` for embedded single quotes.

### Challenge 2
> Several languages use angle brackets for generics and also have a >> right
> shift operator. This led to a classic problem in early versions of C++:
>
> `vector<vector<string>> nestedVectors;`
>
> Later versions of C++ are smarter and can handle the above code. Java and C#
> never had the problem. How do those languages specify and implement this?

Hard **decline**.  The answer is going to be "using a more complex parser."

### Challenge 3
> Name a few contextual keywords from other languages, and the context where
they are meaningful.

**Declined**.  I actually can't think of any examples off the top of my head.

> How would you implement them in your language’s front end if you needed to?

**Declined**.  I might intend to find out, depending on how complex I want to make things later.
