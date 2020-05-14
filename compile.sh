#!/bin/sh

tcc -lm -Wall -g -o bin/clox src/*.c src/ext/*.c
