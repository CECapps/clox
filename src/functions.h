#ifndef cc_functions_h
#define cc_functions_h

#include "object.h"
#include "table.h"
#include "value.h"
#include "vm.h"

void cc_register_functions();
Value cc_function_string_substring(int arg_count, Value* args);
Value cc_function_string_length(int arg_count, Value* args);
Value cc_function_debug_dump_stack(int arg_count, Value* args);
Value cc_function_time(int arg_count, Value* args);
Value cc_function_val_is_empty(int arg_count, Value* args);
Value cc_function_val_is_string(int arg_count, Value* args);
Value cc_function_val_is_number(int arg_count, Value* args);
Value cc_function_val_is_boolean(int arg_count, Value* args);
Value cc_function_val_is_nan(int arg_count, Value* args);
Value cc_function_val_is_infinity(int arg_count, Value* args);

#endif
