#!/bin/sh

tcc -lm -Wall -g -o bin/clox src/*.c src/ext/*.c
#clang -lm -Wall -g -o bin/clox src/*.c src/ext/*.c
#clang -lm -Wall -O3 -o bin/clox src/*.c src/ext/*.c
