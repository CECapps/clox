#include <stdio.h>

#include "debug.h"
#include "value.h"


void disassembleChunk(Chunk* chunk, const char* name) {
  printf("== Disassemble Chunk: %s ==\n", name);

  for (int offset = 0; offset < chunk->count;) {
    offset = disassembleInstruction(chunk, offset);
  }

  printf("\nConstants:\n");
  for (int i = 0; i < chunk->constants.count; i++) {
    printf("\t%d: ", i);
    printValue(chunk->constants.values[i]);
    printf("\n");
  }

  printf("== %s: done ==\n", name);
}


static int constantInstruction(const char* name, Chunk* chunk, int offset) {
  uint8_t constant = chunk->code[offset + 1];
  printf("%-16s C%4d '", name, constant);
  printValue(chunk->constants.values[constant]);
  printf("'\n");

/*
14.5.3
  Remember that disassembleInstruction() also returns a number to tell the caller
  how many bytes to advance to reach the beginning of the next instruction.
  Where OP_RETURN was only a single byte, OP_CONSTANT is two—one for the opcode
  and one for the operand.
*/
  return offset + 2;
}


static int simpleInstruction(const char* name, int offset) {
  printf("%s\n", name);
  return offset + 1;
}


static int byteInstruction(const char* name, Chunk* chunk, int offset) {
  uint8_t slot = chunk->code[offset + 1];
  printf("%-16s b%4d\n", name, slot);
  return offset + 2;
}


static int jumpInstruction(const char* name, int sign, Chunk* chunk, int offset) {
  uint16_t jump = (uint16_t)(chunk->code[offset + 1] << 8);
  jump |= chunk->code[offset + 2];
  printf("%-16s o%4d -> %d\n", name, offset, offset + 3 + sign * jump);
  return offset + 3;
}


int disassembleInstruction(Chunk* chunk, int offset) {
  printf("%04d ", offset);

  if (offset > 0 && chunk->lines[offset] == chunk->lines[offset - 1]) {
    printf("   | ");
  } else {
    printf("%4d ", chunk->lines[offset]);
  }

  uint8_t instruction = chunk->code[offset];
  switch (instruction) {

    case OP_CONSTANT:
      return constantInstruction("OP_CONSTANT", chunk, offset);

    case OP_NIL:
      return simpleInstruction("OP_NIL", offset);

    case OP_TRUE:
      return simpleInstruction("OP_TRUE", offset);

    case OP_FALSE:
      return simpleInstruction("OP_FALSE", offset);

    case OP_POP:
      return simpleInstruction("OP_POP", offset);

    case OP_GET_LOCAL:
      return byteInstruction("OP_GET_LOCAL", chunk, offset);

    case OP_SET_LOCAL:
      return byteInstruction("OP_SET_LOCAL", chunk, offset);

    case OP_GET_GLOBAL:
      return constantInstruction("OP_GET_GLOBAL", chunk, offset);

    case OP_DEFINE_GLOBAL:
      return constantInstruction("OP_DEFINE_GLOBAL", chunk, offset);

    case OP_SET_GLOBAL:
      return constantInstruction("OP_SET_GLOBAL", chunk, offset);

    case OP_EQUAL:
      return simpleInstruction("OP_EQUAL", offset);

    case OP_GREATER:
      return simpleInstruction("OP_GREATER", offset);

    case OP_LESS:
      return simpleInstruction("OP_LESS", offset);

    case OP_ADD:
      return simpleInstruction("OP_ADD", offset);

    case OP_SUBTRACT:
      return simpleInstruction("OP_SUBTRACT", offset);

    case OP_MULTIPLY:
      return simpleInstruction("OP_MULTIPLY", offset);

    case OP_DIVIDE:
      return simpleInstruction("OP_DIVIDE", offset);

    case OP_NOT:
      return simpleInstruction("OP_NOT", offset);

    case OP_NEGATE:
      return simpleInstruction("OP_NEGATE", offset);

    case OP_PRINT:
      return simpleInstruction("OP_PRINT", offset);

    case OP_JUMP:
      return jumpInstruction("OP_JUMP", 1, chunk, offset);

    case OP_JUMP_IF_FALSE:
      return jumpInstruction("OP_JUMP_IF_FALSE", 1, chunk, offset);

    case OP_LOOP:
      return jumpInstruction("OP_LOOP", -1, chunk, offset);

    case OP_CALL:
      return byteInstruction("OP_CALL", chunk, offset);

    case OP_RETURN:
      return simpleInstruction("OP_RETURN", offset);

#ifdef CC_FEATURES
    case OP_EXIT:
      return simpleInstruction("OP_EXIT", offset);

    case OP_ECHO:
      return byteInstruction("OP_ECHO", chunk, offset);

    case OP_TRANSCLUDE:
      return simpleInstruction("OP_TRANSCLUDE", offset);
#endif

    default:
      printf("Unknown opcode %d\n", instruction);
      return offset + 1;
  }
}
