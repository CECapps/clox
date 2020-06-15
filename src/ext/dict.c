#include "../common.h"
#include "../memory.h"
#include "../vm.h"

#include "ferrors.h"

Value cc_function_dict_create(int arg_count, Value* args) { return NIL_VAL; }
Value cc_function_dict_set(int arg_count, Value* args) { return NIL_VAL; }
Value cc_function_dict_update(int arg_count, Value* args) { return NIL_VAL; }
Value cc_function_dict_has(int arg_count, Value* args) { return NIL_VAL; }
Value cc_function_dict_get(int arg_count, Value* args) { return NIL_VAL; }
Value cc_function_dict_remove(int arg_count, Value* args) { return NIL_VAL; }
Value cc_function_dict_count(int arg_count, Value* args) { return NIL_VAL; }
Value cc_function_dict_clear(int arg_count, Value* args) { return NIL_VAL; }
Value cc_function_dict_clone(int arg_count, Value* args) { return NIL_VAL; }
Value cc_function_dict_keys(int arg_count, Value* args) { return NIL_VAL; }
Value cc_function_dict_values(int arg_count, Value* args) { return NIL_VAL; }
Value cc_function_dict_first_key(int arg_count, Value* args) { return NIL_VAL; }
Value cc_function_dict_last_key(int arg_count, Value* args) { return NIL_VAL; }
Value cc_function_dict_find(int arg_count, Value* args) { return NIL_VAL; }
Value cc_function_dict_map(int arg_count, Value* args) { return NIL_VAL; }
Value cc_function_dict_filter(int arg_count, Value* args) { return NIL_VAL; }
Value cc_function_dict_reduce(int arg_count, Value* args) { return NIL_VAL; }
Value cc_function_dict_sort(int arg_count, Value* args) { return NIL_VAL; }
Value cc_function_dict_sort_callback(int arg_count, Value* args) { return NIL_VAL; }
Value cc_function_dict_shuffle(int arg_count, Value* args) { return NIL_VAL; }


void cc_register_ext_dict() {
    defineNative("dict_create",        cc_function_dict_create);

    defineNative("dict_set",           cc_function_dict_set);
    defineNative("dict_update",        cc_function_dict_update);
    defineNative("dict_has",           cc_function_dict_has);
    defineNative("dict_get",           cc_function_dict_get);
    defineNative("dict_remove",        cc_function_dict_remove);
    defineNative("dict_count",         cc_function_dict_count);
    defineNative("dict_clear",         cc_function_dict_clear);

    defineNative("dict_clone",         cc_function_dict_clone);

    defineNative("dict_keys",          cc_function_dict_keys);
    defineNative("dict_values",        cc_function_dict_values);
    defineNative("dict_first_key",     cc_function_dict_first_key);
    defineNative("dict_last_key",      cc_function_dict_last_key);
    defineNative("dict_find",          cc_function_dict_find);

    defineNative("dict_map",           cc_function_dict_map);
    defineNative("dict_filter",        cc_function_dict_filter);
    defineNative("dict_reduce",        cc_function_dict_reduce);

    defineNative("dict_sort",          cc_function_dict_sort);
    defineNative("dict_sort_callback", cc_function_dict_sort_callback);
    defineNative("dict_shuffle",       cc_function_dict_shuffle);
}

