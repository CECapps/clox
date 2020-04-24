# `clox`

> This is what happens when you write your VM in a language that was designed
> to be compiled on a PDP-11.


This is an implementation of `clox` as described in the book [Crafting Interpreters][1].

I am taking up this project as a personal challenge.  I have previously written
trivial virtual machines and state machine-based parsers in high level languages,
but I've never worked with C or any other language that requires manual memory
management.

Each chapter of the book ends with a series of challenges.  I will be reviewing
each challenge.

This code is built for Debian Buster inside WSL.  The target compiler is [`tcc`][2].

Build by invoking `tcc -Wall -o bin/clox src/*.c`

 [1]: https://craftinginterpreters.com
 [2]: http://www.tinycc.org/
