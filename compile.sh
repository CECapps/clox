#!/bin/sh

tcc -lm -DCC_FEATURES -Wall -o bin/clox src/*.c src/ext/*.c
