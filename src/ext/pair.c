#include "../common.h"
#include "../memory.h"
#include "../vm.h"

#include "ferrors.h"

Value cc_function_pair_create(int arg_count, Value* args) { return NIL_VAL; }
Value cc_function_pair_get_left(int arg_count, Value* args) { return NIL_VAL; }
Value cc_function_pair_get_right(int arg_count, Value* args) { return NIL_VAL; }
Value cc_function_pair_set_left(int arg_count, Value* args) { return NIL_VAL; }
Value cc_function_pair_set_right(int arg_count, Value* args) { return NIL_VAL; }
Value cc_function_pair_clone(int arg_count, Value* args) { return NIL_VAL; }
Value cc_function_pair_clear(int arg_count, Value* args) { return NIL_VAL; }

void cc_register_ext_pair() {
    defineNative("pair_create",    cc_function_pair_create);
    defineNative("pair_get_left",  cc_function_pair_get_left);
    defineNative("pair_get_right", cc_function_pair_get_right);
    defineNative("pair_set_left",  cc_function_pair_set_left);
    defineNative("pair_set_right", cc_function_pair_set_right);
    defineNative("pair_clone",     cc_function_pair_clone);
    defineNative("pair_clear",     cc_function_pair_clear);
}
