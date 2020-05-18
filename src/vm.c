#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <stdlib.h>

#include "common.h"
#include "compiler.h"
#include "debug.h"
#include "object.h"
#include "memory.h"
#include "vm.h"

#ifdef CC_FEATURES
#include "ext/functions.h"
#include "ext/userhash.h"
#include "ext/userarray.h"
#include "ext/ferrors.h"
#endif

VM vm;


static Value clockNative(int argCount, Value* args) {
  return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}


static void resetStack() {
  vm.stackTop = vm.stack;
  vm.frameCount = 0;
}


static void runtimeError(const char* format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputs("\n", stderr);

  for (int i = vm.frameCount - 1; i >= 0; i--) {
    CallFrame* frame = &vm.frames[i];
    ObjFunction* function = frame->function;
    // -1 because the IP is sitting on the next instruction to be
    // executed.
    size_t instruction = frame->ip - function->chunk.code - 1;
    fprintf(stderr, "[line %d] in ", function->chunk.lines[instruction]);
    if (function->name == NULL) {
      fprintf(stderr, "script\n");
    } else {
      fprintf(stderr, "%s()\n", function->name->chars);
    }
  }

  resetStack();
}

#ifdef CC_FEATURES
void defineNative(const char* name, NativeFn function) {
#else
static void defineNative(const char* name, NativeFn function) {
#endif
  ObjString* native_name = copyString(name, (int)strlen(name));
  push(OBJ_VAL(native_name));
  push(OBJ_VAL(newNative(function, native_name)));
  tableSet(&vm.globals, native_name, vm.stack[1]);
  pop();
  pop();
}


void initVM() {
  resetStack();
  vm.objects = NULL;
  initTable(&vm.globals);
  initTable(&vm.strings);

  defineNative("clock", clockNative);

#ifdef CC_FEATURES
  cc_register_ext_functions();
  cc_register_ext_userhash();
  cc_register_ext_userarray();
#endif
}


void freeVM() {
  freeTable(&vm.globals);
  freeTable(&vm.strings);
  freeObjects();
}


void push(Value value) {
  *vm.stackTop = value;
  vm.stackTop++;
}


Value pop() {
  vm.stackTop--;
  return *vm.stackTop;
}


static Value peek(int distance) {
  return vm.stackTop[-1 - distance];
}


static bool call(ObjFunction* function, int argCount) {
  if (argCount != function->arity) {
    runtimeError("Expected %d arguments but got %d.", function->arity, argCount);
    return false;
  }

  if (vm.frameCount == FRAMES_MAX) {
    runtimeError("Stack overflow.");
    return false;
  }

  CallFrame* frame = &vm.frames[vm.frameCount++];
  frame->function = function;
  frame->ip = function->chunk.code;

  frame->slots = vm.stackTop - argCount - 1;
  return true;
}


static bool callValue(Value callee, int argCount) {
  if (IS_OBJ(callee)) {
    switch (OBJ_TYPE(callee)) {

      case OBJ_FUNCTION:
        return call(AS_FUNCTION(callee), argCount);

      case OBJ_NATIVE: {
        ObjNative *native = AS_NATIVE(callee);
        NativeFn func = native->function;
        Value result = func(argCount, vm.stackTop - argCount);
        bool had_error = false;
#ifdef CC_FEATURES
        if(IS_FERROR(result)) {
          had_error = true;
          ObjFunctionError* err = AS_FERROR(result);
          if(err->sys_errno == 0) {
            runtimeError(
              "%s(): %s",
              native->name->chars,
              cc_ferror_to_string(err->ferror_id)
            );
          } else {
            runtimeError(
              "%s(): %s: %s",
              native->name->chars,
              cc_ferror_to_string(err->ferror_id),
              strerror(err->sys_errno)
            );
          }
          result = NIL_VAL;
        }
#endif
        vm.stackTop -= argCount + 1;
        push(result);
        return !had_error;
      }

      default:
        // Non-callable object type.
        break;

    }
  }

  runtimeError("Can only call functions and classes.");
  return false;
}


#ifdef CC_FEATURES
static InterpretResult run(int until_frame); // lol

Value callCallback(Value callback, int argCount, Value* args) {
  if(!IS_OBJ(callback) || !IS_FUNCTION(callback)) {
    runtimeError("Callback must be a function.");
    return NIL_VAL;
  }

  int current_frame = vm.frameCount;
  // call() below expects the function to be called to already be on the stack,
  // folllowed by its arguments.  Set that up now.
  push(callback);
  for(int i = 0; i < argCount; i++) {
    push(args[i]);
  }

  // call() works by manipulating the call frame and then moving the VM program
  // counter to the start of the function body.  True means that we can continue.
  bool success = call(AS_FUNCTION(callback), argCount);
  if(!success) {
    // call() will only return false if a runtime error was reported.
    return NIL_VAL;
  }
  // callCallback happens from inside a native function.  We're going to fire
  // the VM back up and resume execution from within the callback, stopping when
  // a return happens and we're back at the frame we started at.
  InterpretResult res = run(current_frame);

  if(res != INTERPRET_OK) {
    // Some sort of runtime error happened.  Assume the return value is bogus.
    return NIL_VAL;
  }
  return pop();
}
#endif


static bool isFalsey(Value value) {
  return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}


static void concatenate() {
  ObjString* b = AS_STRING(pop());
  ObjString* a = AS_STRING(pop());

  int length = a->length + b->length;
  char* chars = ALLOCATE(char, length + 1);
  memcpy(chars, a->chars, a->length);
  memcpy(chars + a->length, b->chars, b->length);
  chars[length] = '\0';

  ObjString* result = takeString(chars, length);
  push(OBJ_VAL(result));
}

#ifdef CC_FEATURES
static InterpretResult run(int until_frame) {
#else
static InterpretResult run() {
#endif
  CallFrame* frame = &vm.frames[vm.frameCount - 1];

#define READ_BYTE() (*frame->ip++)
#define READ_SHORT() (frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))
#define READ_CONSTANT() (frame->function->chunk.constants.values[READ_BYTE()])
#define READ_STRING() AS_STRING(READ_CONSTANT())

#define BINARY_OP(valueType, op) \
    do { \
      if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
        runtimeError("Operands must be numbers."); \
        return INTERPRET_RUNTIME_ERROR; \
      } \
      \
      double b = AS_NUMBER(pop()); \
      double a = AS_NUMBER(pop()); \
      push(valueType(a op b)); \
    } while (false)

// @TODO Hey, why is this a for instead of a while(true)
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
    disassembleInstruction(&frame->function->chunk, (int)(frame->ip - frame->function->chunk.code));
#endif

    uint8_t instruction;
    switch (instruction = READ_BYTE()) {

      case OP_CONSTANT: {
        Value constant = READ_CONSTANT();
        push(constant);
        break;
      }

      case OP_NIL:      push(NIL_VAL); break;
      case OP_TRUE:     push(BOOL_VAL(true)); break;
      case OP_FALSE:    push(BOOL_VAL(false)); break;

      case OP_POP:  pop(); break;

      case OP_GET_LOCAL: {
        uint8_t slot = READ_BYTE();
        push(frame->slots[slot]);
        break;
      }

      case OP_SET_LOCAL: {
        uint8_t slot = READ_BYTE();
        frame->slots[slot] = peek(0);
        break;
      }

      case OP_GET_GLOBAL: {
        ObjString* name = READ_STRING();
        Value value;
        if (!tableGet(&vm.globals, name, &value)) {
          runtimeError("Undefined variable '%s'.", name->chars);
          return INTERPRET_RUNTIME_ERROR;
        }
        push(value);
        break;
      }

      case OP_DEFINE_GLOBAL: {
        ObjString* name = READ_STRING();
        tableSet(&vm.globals, name, peek(0));
        pop();
        break;
      }

      case OP_SET_GLOBAL: {
        ObjString* name = READ_STRING();
        if (tableSet(&vm.globals, name, peek(0))) {
          tableDelete(&vm.globals, name);
          runtimeError("Undefined variable '%s'.", name->chars);
          return INTERPRET_RUNTIME_ERROR;
        }
        break;
      }

      case OP_EQUAL: {
        Value b = pop();
        Value a = pop();
        push(BOOL_VAL(valuesEqual(a, b)));
        break;
      }

      case OP_GREATER:  BINARY_OP(BOOL_VAL, >);   break;
      case OP_LESS:     BINARY_OP(BOOL_VAL, <);   break;
      case OP_ADD: {
        if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
          concatenate();
        } else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
          double b = AS_NUMBER(pop());
          double a = AS_NUMBER(pop());
          push(NUMBER_VAL(a + b));
        } else {
          runtimeError("Operands must be two numbers or two strings.");
          return INTERPRET_RUNTIME_ERROR;
        }
        break;
      }
      case OP_SUBTRACT: BINARY_OP(NUMBER_VAL, -); break;
      case OP_MULTIPLY: BINARY_OP(NUMBER_VAL, *); break;
      case OP_DIVIDE:   BINARY_OP(NUMBER_VAL, /); break;
      case OP_NOT:
        push(BOOL_VAL(isFalsey(pop())));
        break;

      case OP_NEGATE:
        if (!IS_NUMBER(peek(0))) {
          runtimeError("Operand must be a number.");
          return INTERPRET_RUNTIME_ERROR;
        }

        push(NUMBER_VAL(-AS_NUMBER(pop())));
        break;
        break;

      case OP_PRINT: {
        printValue(pop());
        printf("\n");
        break;
      }

      case OP_JUMP: {
        uint16_t offset = READ_SHORT();
        frame->ip += offset;
        break;
      }

      case OP_JUMP_IF_FALSE: {
        uint16_t offset = READ_SHORT();
        if (isFalsey(peek(0))) frame->ip += offset;
        break;
      }

      case OP_LOOP: {
        uint16_t offset = READ_SHORT();
        frame->ip -= offset;
        break;
      }

      case OP_CALL: {
        int argCount = READ_BYTE();
        if (!callValue(peek(argCount), argCount)) {
          return INTERPRET_RUNTIME_ERROR;
        }
        frame = &vm.frames[vm.frameCount - 1];
        break;
      }

      case OP_RETURN: {
        Value result = pop();

        vm.frameCount--;
        if (vm.frameCount == 0) {
          pop();
          return INTERPRET_OK; // This exits.
        }

        vm.stackTop = frame->slots;
        push(result);

        frame = &vm.frames[vm.frameCount - 1];

#ifdef CC_FEATURES
        // If we're inside of a callback, return control to the calling context
        // once we've hit the original calling frame.
        if(vm.frameCount == until_frame) {
          return INTERPRET_OK;
        }
#endif

        break;
      }

#ifdef CC_FEATURES
      case OP_EXIT: {
        // POSIX says to only use 8 bits out of the 16 bit ("int" type) exit value
        double errorlevel = AS_NUMBER(pop());
        if(errorlevel > 255 || errorlevel < 0) {
          runtimeError("Exit value must be between 0 and 255, inclusive.");
          return INTERPRET_RUNTIME_ERROR;
        }

        // This is a clean exit.  Tear down the environment to let the (future)
        // GC clean things up, just in case.
        // @FIXME: I tried moving the call to freeVM() to an atexit() function
        // over in main, but the compiler threw an error at me that suggested
        // some deep level library weirdness that SO in turn said I should just
        // stop trying to worry about and Don't Do That.
        freeVM();

        // @FIXME: I'd like to not exit from this deep inside the code.  What
        // would be a better way to handle this?
        exit( (int)errorlevel );
      }
#endif

#ifdef CC_FEATURES
      case OP_ECHO: {
        uint8_t arg_count = READ_BYTE();
        uint8_t pops = 0;
        while(arg_count-- > 0) {
          printValue(peek(arg_count));
          pops++;
        }
        while(pops-- > 0) {
          pop();
        }
        break;
      }
#endif

      case OP_TRANSCLUDE: {
        // Our work was done in the compiler.  The filename that was transacluded
        // is forced into the chunk because of how the string parser works, so
        // let's clean that up now.  @FIXME This is dumb.
        pop();
        break;
      }

      // Variation
      default: {
        runtimeError("Unknown opcode %d.", instruction);
        return INTERPRET_RUNTIME_ERROR;
      }

    }
  }

#undef READ_BYTE
#undef READ_SHORT
#undef READ_CONSTANT
#undef READ_STRING
#undef BINARY_OP
}


InterpretResult interpret(const char* source, int starting_line) {
  ObjFunction* function = compile(source, starting_line);
  if (function == NULL) return INTERPRET_COMPILE_ERROR;

  push(OBJ_VAL(function));
  callValue(OBJ_VAL(function), 0);

#ifdef CC_FEATURES
  return run(0);
#else
  return run();
#endif
}
