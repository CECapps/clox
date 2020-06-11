#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <math.h>

#include "../common.h"
#include "../vm.h"

#include "./number.h"
#include "./string.h"
#include "./file.h"
#include "./process.h"
#include "./userarray.h"
#include "./ferrors.h"

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


Value cc_function_debug_dump_value_hash(int arg_count, Value* args) {
    if(arg_count != 1) { return FERROR_VAL(FE_ARG_COUNT_1); }

    Value v = args[0];
    uint64_t hash = v.as.bits;

    // Strings have their own hash precomputed, use it when we can.
    if (IS_STRING(v)) {
        return NUMBER_VAL(AS_STRING(v)->hash);
    }

    // It happens that nil, false, and 0 are all represented by zero!  Oops.
    // There are a small number of Value types, stored as an enum starting at
    // zero.  Adding 1 and then the type bitshifted over leads to nil, false,
    // true, and zero all having different base values.  This causes no problems
    // for the double underlying the number type.
    if (IS_NIL(v) || IS_BOOL(v) || IS_NUMBER(v)) {
        hash += 1 + (((uint8_t)v.type) << 4);
    }

    // At this point, we have a raw hash value comprised of the binary data
    // for the underlying Value type.  This raw value will be unique per Value
    // in the system.  Time to hide this behind a hash!
    // One round of splittable64 via https://nullprogram.com/blog/2018/07/31/
    hash ^= hash >> 30;
    hash *= UINT64_C(0xbf58476d1ce4e5b9);
    hash ^= hash >> 27;
    hash *= UINT64_C(0x94d049bb133111eb);
    hash ^= hash >> 31;

    // All other code that expects a hash value uses 32 bits, so we will also.
    uint32_t crunched = (hash >> 32) ^ hash;

    return NUMBER_VAL(crunched);
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
        return FERROR_VAL(FE_ARG_COUNT_1);
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
    } else if(IS_USERARRAY(args[0]) && AS_USERARRAY(args[0])->inner.count == 0) {
        // Empty arrays are empty.
        return BOOL_VAL(true);
    } else if(IS_USERHASH(args[0]) && AS_USERHASH(args[0])->table.count == 0) {
        // Empty hashes are considered empty.
        return BOOL_VAL(true);
    }

    // Everything else is not empty, including but not limited to functions,
    // classes, NaN, filehandles, booleans (yes, false is not empty), etc.
    return BOOL_VAL(false);
}


Value cc_function_val_is_string(int arg_count, Value* args) {
    if(arg_count != 1) {
        return FERROR_VAL(FE_ARG_COUNT_1);
    }
    return BOOL_VAL(IS_STRING(args[0]));
}


Value cc_function_val_is_number(int arg_count, Value* args) {
    if(arg_count != 1) {
        return FERROR_VAL(FE_ARG_COUNT_1);
    }
    return BOOL_VAL(IS_NUMBER(args[0]));
}


Value cc_function_val_is_boolean(int arg_count, Value* args) {
    if(arg_count != 1) {
        return FERROR_VAL(FE_ARG_COUNT_1);
    }
    return BOOL_VAL(IS_BOOL(args[0]));
}


Value cc_function_val_is_array(int arg_count, Value* args) {
    if(arg_count != 1) {
        return FERROR_VAL(FE_ARG_COUNT_1);
    }
    return BOOL_VAL(IS_USERARRAY(args[0]));
}


Value cc_function_val_is_hash(int arg_count, Value* args) {
    if(arg_count != 1) {
        return FERROR_VAL(FE_ARG_COUNT_1);
    }
    return BOOL_VAL(IS_USERHASH(args[0]));
}


Value cc_function_val_is_filehandle(int arg_count, Value* args) {
    if(arg_count != 1) {
        return FERROR_VAL(FE_ARG_COUNT_1);
    }
    return BOOL_VAL(IS_FILEHANDLE(args[0]));
}


Value cc_function_val_is_nan(int arg_count, Value* args) {
    if(arg_count != 1) {
        return FERROR_VAL(FE_ARG_COUNT_1);
    }
    if(!IS_NUMBER(args[0])) {
        return BOOL_VAL(false);
    }
    return BOOL_VAL( isnan(AS_NUMBER(args[0])) != 0 );
}


Value cc_function_val_is_infinity(int arg_count, Value* args) {
    if(arg_count != 1) {
        return FERROR_VAL(FE_ARG_COUNT_1);
    }
    if(!IS_NUMBER(args[0])) {
        return BOOL_VAL(false);
    }
    return BOOL_VAL( isinf(AS_NUMBER(args[0])) != 0 );
}


extern int global_argc;
extern const char** global_argv;
Value cc_function_environment_arguments(int arg_count, Value* args) {

    ObjUserArray* ua = newUserArray();
    int index = 0;
    for(int i = 1; i < global_argc; i++) {
        ua_grow(ua, index);
        ua->inner.values[index++] = OBJ_VAL(copyString(global_argv[i], strlen(global_argv[i])));
    }
    ua->inner.count = index;

    return OBJ_VAL(ua);
}


void cc_register_ext_functions() {
  defineNative("debug_dump_stack",      cc_function_debug_dump_stack);
  defineNative("debug_dump_value_hash", cc_function_debug_dump_value_hash);
  defineNative("time",                  cc_function_time);
  defineNative("environment_getvar",    cc_function_environment_getvar);
  defineNative("environment_arguments", cc_function_environment_arguments);



  defineNative("val_is_empty",          cc_function_val_is_empty);
  defineNative("val_is_string",         cc_function_val_is_string);
  defineNative("val_is_number",         cc_function_val_is_number);
  defineNative("val_is_boolean",        cc_function_val_is_boolean);
  defineNative("val_is_array",          cc_function_val_is_array);
  defineNative("val_is_hash",           cc_function_val_is_hash);
  defineNative("val_is_filehandle",     cc_function_val_is_filehandle);
  defineNative("val_is_nan",            cc_function_val_is_nan);
  defineNative("val_is_infinity",       cc_function_val_is_infinity);

  cc_register_ext_number();
  cc_register_ext_string();
  cc_register_ext_file();
  cc_register_ext_process();
}
