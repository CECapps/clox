#ifndef clox_vm_h
#define clox_vm_h

#include "object.h"
#include "table.h"
#include "value.h"

#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)


typedef struct {
  ObjFunction* function;
  uint8_t* ip;
  Value* slots;
} CallFrame;


typedef struct {
  CallFrame frames[FRAMES_MAX];
  int frameCount;

  Value stack[STACK_MAX];
  Value* stackTop;
  Table globals;
  Table strings;

  Obj* objects;
} VM;


typedef enum {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR
} InterpretResult;


extern VM vm;


void initVM();
void freeVM();
InterpretResult interpret(const char* source, int starting_line);
void push(Value value);
Value pop();

#ifdef CC_FEATURES
void defineNative(const char* name, NativeFn function);
Value callCallback(Value callback, int argCount, Value* args);
#endif


#endif
