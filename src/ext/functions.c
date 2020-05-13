#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <sys/time.h>
#include <math.h>
#include <fenv.h>

#include "../common.h"
#include "../compiler.h"
#include "../debug.h"
#include "../object.h"
#include "../memory.h"
#include "../table.h"
#include "../vm.h"

#include "./number.h"
#include "./string.h"
#include "./file.h"

Value cc_function_debug_dump_stack(int arg_count, Value* args) {
  int counter = 0;
  for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
    printf("%d:", counter);
    printValue(*slot);
    printf(", ");
    counter++;
  }
  printf("\n");
  return NIL_VAL;
}


Value cc_function_time(int arg_count, Value* args) {
  struct timeval current_time;
  gettimeofday(&current_time, /* timezone */ NULL);
  return NUMBER_VAL((double)(current_time.tv_sec) + ((double)(current_time.tv_usec) / 1000000));
}


Value cc_function_environment_getvar(int arg_count, Value* args) {
    if(arg_count != 1) {
        return NIL_VAL;
    }
    char * env_var = getenv(AS_CSTRING(args[0]));

    if(env_var == NULL) {
        return BOOL_VAL(false);
    }

    return OBJ_VAL(copyString(env_var, strlen(env_var)));
}


Value cc_function_val_is_empty(int arg_count, Value* args) {
    if(arg_count != 1) {
        return NIL_VAL;
    }

    if(IS_NIL(args[0])) {
        // Nil never has a value, so it is always empty.
        return BOOL_VAL(true);
    } else if(IS_STRING(args[0]) && AS_STRING(args[0])->length == 0) {
        // Only the empty string is considered empty.  Whitespace is not trimmed.
        return BOOL_VAL(true);
    } else if(IS_NUMBER(args[0]) && AS_NUMBER(args[0]) == 0.0) {
        // Only zero is considered empty.
        return BOOL_VAL(true);
    }

    // Only the three above things are considered empty.  All other values are
    // considered not empty, including booleans and function/class types.
    return BOOL_VAL(false);
}


Value cc_function_val_is_string(int arg_count, Value* args) {
    if(arg_count != 1) {
        return NIL_VAL;
    }
    return BOOL_VAL(IS_STRING(args[0]));
}


Value cc_function_val_is_number(int arg_count, Value* args) {
    if(arg_count != 1) {
        return NIL_VAL;
    }
    return BOOL_VAL(IS_NUMBER(args[0]));
}


Value cc_function_val_is_boolean(int arg_count, Value* args) {
    if(arg_count != 1) {
        return NIL_VAL;
    }
    return BOOL_VAL(IS_BOOL(args[0]));
}


Value cc_function_val_is_nan(int arg_count, Value* args) {
    if(arg_count != 1) {
        return NIL_VAL;
    }
    if(!IS_NUMBER(args[0])) {
        return BOOL_VAL(false);
    }
    return BOOL_VAL( isnan(AS_NUMBER(args[0])) != 0 );
}


Value cc_function_val_is_infinity(int arg_count, Value* args) {
    if(arg_count != 1) {
        return NIL_VAL;
    }
    if(!IS_NUMBER(args[0])) {
        return BOOL_VAL(false);
    }
    return BOOL_VAL( isinf(AS_NUMBER(args[0])) != 0 );
}


void cc_register_ext_functions() {
  defineNative("debug_dump_stack",      cc_function_debug_dump_stack);
  defineNative("time",                  cc_function_time);
  defineNative("environment_getvar",    cc_function_environment_getvar);

  defineNative("val_is_empty",          cc_function_val_is_empty);
  defineNative("val_is_string",         cc_function_val_is_string);
  defineNative("val_is_number",         cc_function_val_is_number);
  defineNative("val_is_boolean",        cc_function_val_is_boolean);
  defineNative("val_is_nan",            cc_function_val_is_nan);
  defineNative("val_is_infinity",       cc_function_val_is_infinity);

  cc_register_ext_number();
  cc_register_ext_string();
  cc_register_ext_file();
}
