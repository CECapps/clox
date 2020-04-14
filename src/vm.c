#include <stdio.h>

#include "common.h"
#include "debug.h"
#include "vm.h"

VM vm;

// "static" here creates a function that is not visible to the outside world.
// This function is also not declared in vm.h.
static void resetStack() {
  vm.stackTop = vm.stack;
}

void initVM() {
  resetStack();
}

void freeVM() {
}

void push(Value value) {
  *vm.stackTop = value;
  vm.stackTop++;
}

Value pop() {
  vm.stackTop--;
  return *vm.stackTop;
}

static InterpretResult run() {
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])

  for (;;) {

#ifdef DEBUG_TRACE_EXECUTION
    bool found_things_on_stack = false; // Variation - explicit denation of an empty stack
    printf("   Stack: "); // Variation - this is supposed to be just 10 spaces
    for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
      found_things_on_stack = true; // Variation
      printf("[ ");
      printValue(*slot);
      printf(" ]");
    }

    // Variation
    if (!found_things_on_stack) {
      printf("(empty)");
    }

    printf("\n");
    disassembleInstruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
#endif

    uint8_t instruction;
    switch (instruction = READ_BYTE()) {

      case OP_CONSTANT: {
        Value constant = READ_CONSTANT();
        push(constant);
        break;
      }

      case OP_NEGATE:
        push(-pop());
        break;

      case OP_RETURN: {
        printValue(pop());
        printf("\n");
        return INTERPRET_OK;
      }

    }
  }

#undef READ_BYTE
#undef READ_CONSTANT
}

InterpretResult interpret(Chunk* chunk) {
  vm.chunk = chunk;
  vm.ip = vm.chunk->code;
  return run();
}
