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

See also Chapter 22, Challenge 4.

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

## Chapter 17, "Compiling Expressions"

### Challenge 1
> Take this (strange) expression:
>
> `(-1 + 2) * 3 - -4`
>
> Write a trace of how those functions are called. Show the order they are
> called, which calls which, and the arguments passed to them.

**Deferred** until I have to add more operators later.

### Challenge 2
> In the full Lox language, what other tokens can be used in both prefix and
> infix positions? What about in C or another language of your choice?

I don't know Lox well enough yet to name other reused tokens.  The `*` operator
is used in C as both multiplication and for pointer purposes. The `&` operator
is used in C as both bitwise-and and for pointer purposes.

### Challenge 3
> Add support for [the `? :`] operator to the compiler.

**Deferred** and **high** priority.  Waiting for operator precedence and if/else.

## Chapter 18, "Types of Values"

### Challenge 1
> We could reduce our binary operators even further than we did here. Which
other instructions can you eliminate, and how would the compiler cope with
their absence?

Greater could be implemented as negated less-or-equal, for example.  Some of the
others would work in the same way.  The compiler would just have to make sure
that the instructions *correctly* negates the operation.

### Challenge 2
> Conversely, we can improve the speed of our bytecode VM by adding more
specific instructions that correspond to higher-level operations. What
instructions would you define to speed up the kind of user code we added
support for in this chapter?

Deferred until there's enough of a language to benchmark opcode changes.

## Chapter 19, "Strings"

### Challenge 1
> A more efficient solution relies on a technique called "flexible array
members". Use that to store the ObjString and its character array in a single
contiguous allocation.

**Declined.**  I'm intending to use an exteral Unicode library to process
strings in the future.  I have no idea whether or not I'll need to handle
memory allocation for those strings myself, or what other restrictions I need
to handle.  Keeping the actual values inside a separate character array is
safer for my future needs.

### Challenge 2
> Instead, we could keep track of which ObjStrings own their character array and
which are “constant strings” that just point back to the original source string
or some other non-freeable location. Add support for this.

**Declined** for the same reason.

### Challenge 3
> If Lox was your language, what would you have it do when a user tries to use
`+` with one string operand and the other some other type? Justify your choice.
What do other languages do?

I intend to move away from using `+` as the concatenation operator.  If the
operands are not numeric, an error should be thrown.  Some languages attempt
to perform dynamic type conversion, which can lead to loss of precision or
loss of data itself.  I am not in favor of lossy type conversion.

Javascript does weird, weird stuff with `+`, a bunch of which can be seen in the
infamous [Wat talk](https://www.destroyallsoftware.com/talks/wat).  Array plus
array is the empty string!?

## Chapter 20, "Hash Tables"

### Challenge 1
> Add support for keys of the other primitive types: numbers, Booleans, and nil.

**Deferred** with **high** priority.  I intend to add hashes to the language,
and will tackle this task then.

> Later, clox will support user-defined classes. If we want to support keys
> that are instances of those classes, what kind of complexity does that add?

First, I'll ignore whatever changes are needed to handle numbers, booleans,
and nil.  I don't think there needs to be a fundamental shift to the way that
the code operates, except that the creation of the hashcode itself would come
from something other than calling the internal hash function on the internal
string representation.

I'm going to need to do this if I ever expect to support Sets, so I guess that
this is also **deferred**.

### Challenge 2
> Look up a few hash table implementations in different open source systems,
> research the choices they made, and try to figure out why they did things
> that way.

**Deferred**.  I'll be looking up what PHP does first, and then possibly look
at Java.  I should also look at Perl, but I'm not sure I want to look at C that
has been created by Perl developers.

### Challenge 3
> Write a handful of different benchmark programs to validate our hash table
> implementation. How does the performance vary between them? Why did you
> choose the specific test cases you chose?

**Deferred** until hashes are in the language.

## Chapter 21, "Global Variables"

### Challenge 1
> The compiler adds a global variable’s name to the constant table as a string
> every time an identifier is encountered. [...] Optimize this.

The same is also true for repeated string literals, or for repeated numbers.
Or bools.  Or nil.  My gut instinct would be to use a hash table here to ensure
uniqueness.

The brute force solution would be to search the constants array during each
insert and only insert if the constant was not already defined.  This would be
a much less radical change for the rest of the code.  However, being brute force,
it's also slow and possibly wasteful.

> How does your optimization affect the performance of the compiler compared
> to the runtime? Is this the right trade-off?

**Deferred** with *medium* priority.  This sounds like an optimization worth
pursuing, but I'm going to (again) wait until hashes are in the language before
thinking about how to benchmark it.

### Challenge 2
> Looking up a global variable by name in a hash table each time it is used is
pretty slow, even with a good hash table. Can you come up with a more efficient
way to store and access global variables without changing the semantics?

This sounds like a job for a [radix tree](https://en.wikipedia.org/wiki/Radix_tree).

**Deferred** priority *medium*.  Need to play around with how they work first.

See also Chapter 22 Challenge 1.

### Challenge 3
> We could report mistakes like [undefined but never used variable] as compile
errors, at least when running from a script. Do you think we should? Justify
your answer. What do other scripting languages you know do?

Very few scripting languages complain about uncalled or unreachable code.  Given
that scripting languages tend to be highly dynamic and have the ability to
inline code via things like `include`/`require`, I'm not sure that any compile
or run time error checking on uncalled functions or undefined variables that
never reach the point of being evaluated is appropriate.

This might be more appropriate for language analysis tools and things like IDEs.

## Chapter 22

### Challenge 1
> [...] when the compiler resolves a reference to a variable, we have to do a
> linear scan through the array.  Come up with something more efficient. Do you
> think the additional complexity is worth it?

The text itself suggests that the mechanisms for local variable lookup could
benefit from being in a hash table.  I agree in principle, but the last chapter
suggested that even a hash table lookup might be slow.  This might also be a
case where a radix tree could be the right solution.

**Deferred** priority *medium*, after Chapeter 21 Challenge 2.

### Challenge 2
> How do other languages handle code like: `var a = a;`? What would you do? Why?

It's legal in Perl and Javascript.  It's an undefined variable notice in PHP,
but that's just a notice, not a script-halting error.

This is one of those cases where it's possibly a coding error, and something at
least as strong as a linting tool should warn about it.  I don't think it should
be an *error* though.

### Challenge 3
> Pick a keyword for a single-assignment variable form to add to Lox. Justify
> your choice, then implement it. An attempt to assign to a variable declared
> using your new keyword should cause a compile error.

I like it, though my gut instinct is that assigning to an unassignable variable
smells more like a runtime error, not a compile error.

**Deferred**, priority *medium*.

### Challenge 4
> Extend clox to allow more than 255 local variables to be in scope at a time.

This smells like a variation on the same thing that would need to happen to
the stack, with regard to resizing on demand.  At least in spirit.  The two
sections of code don't really work similarly.

**Deferred** until after Chapter 15, Challenge 3.
