#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"

// 14.3.0 - List of opcodes
typedef enum {
    OP_RETURN,
} OpCode;

// 14.3.1 - Chunks are containers for bytecode
typedef struct {
    int count;
    int capacity;
    uint8_t* code;
} Chunk;

void initChunk(Chunk* chunk);
void freeChunk(Chunk* chunk);
void writeChunk(Chunk* chunk, uint8_t byte);

#endif
