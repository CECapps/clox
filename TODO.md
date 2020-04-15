# TODO
 - REPL
   - switch to readline/libedit/whatevs
   - add "exit"
 - Chapter 14
    - Challenge 1
        - Skipping entirely.
    - Challenge 2
        - "many instruction sets feature multiple instructions that perform the same operation but with operands of different sizes. Leave our existing one-byte `OP_CONSTANT` instruction alone, and define a second `OP_CONSTANT_LONG` instruction. It stores the operand as a 24-bit number, which should be plenty."
        - From reading ahead, I recall vaguely that something like this ends up being implemented.
        Deferring.  Priority: low
    - Challenge 3
        - Research memory allocation strategies.
        Deferring.  Priority: medium.
 - Chapter 15
   - Challenge 1
     - Deferring until after operator precedence.  Priority: very low.
   - Challenge 3
     - Deferring, short term.  How to do a growable array inside a struct?  Replace it with a pointer?
       **Priority: high.**
 - Chapter 16
   - Challenge 2
     - Skipping entirely.  I do not care how other languages deal with possibly confusing operator combos.
   - Challenge 3.
     - Deferring.  How Lox itself handles `this` is a great example of contextual keywords.  Priority: low.
