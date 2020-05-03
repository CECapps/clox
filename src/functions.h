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

Value cc_function_number_absolute(int arg_count, Value* args);
Value cc_function_number_remainder(int arg_count, Value* args);
Value cc_function_number_minimum(int arg_count, Value* args);
Value cc_function_number_maximum(int arg_count, Value* args);
Value cc_function_number_floor(int arg_count, Value* args);
Value cc_function_number_ceiling(int arg_count, Value* args);
Value cc_function_number_clamp(int arg_count, Value* args);
Value cc_function_number_round(int arg_count, Value* args);
Value cc_function_number_to_string(int arg_count, Value* args);
Value cc_function_number_to_hex_string(int arg_count, Value* args);

Value cc_function_ht_create(int arg_count, Value* args);
Value cc_function_ht_set(int arg_count, Value* args);
Value cc_function_ht_update(int arg_count, Value* args);
Value cc_function_ht_unset(int arg_count, Value* args);
Value cc_function_ht_has(int arg_count, Value* args);
Value cc_function_ht_get(int arg_count, Value* args);
Value cc_function_ht_count_keys(int arg_count, Value* args);
Value cc_function_ht_get_key_index(int arg_count, Value* args);
Value cc_function_ht_clear(int arg_count, Value* args);

#endif
