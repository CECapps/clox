#include <stdio.h>
#include <math.h>

#include "../memory.h"
#include "../object.h"
#include "../value.h"
#include "../vm.h"

#include "number.h"


static void ua_grow(ObjUserArray* ua, int new_capacity) {
    int old_capacity = ua->inner.capacity;
    while(ua->inner.capacity < new_capacity) {
        ua->inner.capacity = GROW_CAPACITY(ua->inner.capacity);
    }
    ua->inner.values = GROW_ARRAY(ua->inner.values, Value, old_capacity, ua->inner.capacity);
    // Initialize each newly allocated Value element as nil.
    for(; old_capacity < ua->inner.capacity; old_capacity++) {
        ua->inner.values[old_capacity] = NIL_VAL;
    }
}

static int16_t ua_normalize_index(ObjUserArray* ua, double target_index, bool valid_indexes_only) {
    int64_t index = (int64_t)target_index;
    if(index < 0) {
        index += ua->inner.count;
    }

    if(index > INT16_MAX || index < 0 || (valid_indexes_only && index > ua->inner.count)) {
        return -1;
    }
    return (int16_t)index;
}


Value cc_function_val_is_userarray(int arg_count, Value* args) {
    if(arg_count != 1) {
        return NIL_VAL;
    }
    return BOOL_VAL( IS_USERARRAY(args[0]) );
}


Value cc_function_ar_create(int arg_count, Value* args) {
    return OBJ_VAL(newUserArray());
}


Value cc_function_ar_set(int arg_count, Value* args) {
    if(arg_count != 3 || !IS_USERARRAY(args[0]) || !IS_NUMBER(args[1])) {
        return NIL_VAL;
    }

    ObjUserArray* ua = AS_USERARRAY(args[0]);
    int16_t target_index = ua_normalize_index(ua, AS_NUMBER(args[1]), false);
    if(target_index < 0) {
        return BOOL_VAL(false);
    }

    // ar_set can be called with an arbritary index, including ones we don't
    // have yet.  If we need to, grow to hold at least target_index values.
    if(target_index >= ua->inner.capacity) {
        ua_grow(ua, target_index + 1);
    }
    ua->inner.values[target_index] = args[2];
    if(target_index >= ua->inner.count) {
        ua->inner.count = target_index + 1;
    }
    return BOOL_VAL(true);
}


Value cc_function_ar_update(int arg_count, Value* args) {
    if(arg_count != 3 || !IS_USERARRAY(args[0]) || !IS_NUMBER(args[1])) {
        return NIL_VAL;
    }

    ObjUserArray* ua = AS_USERARRAY(args[0]);
    int16_t target_index = ua_normalize_index(ua, AS_NUMBER(args[1]), false);
    if(target_index < 0) {
        return BOOL_VAL(false);
    }

    // Like ar_set, ar_update can be called with an index that's out of range.
    if(target_index >= ua->inner.capacity) {
        ua_grow(ua, target_index + 1);
    }
    // ua_grow takes care of initializing previously-out-of-range values to nil.
    Value old_value = ua->inner.values[target_index];
    ua->inner.values[target_index] = args[2];
    if(target_index >= ua->inner.count) {
        ua->inner.count = target_index + 1;
    }
    return old_value;
}


Value cc_function_ar_has(int arg_count, Value* args) {
    if(arg_count != 2 || !IS_USERARRAY(args[0]) || !IS_NUMBER(args[1])) {
        return NIL_VAL;
    }
    ObjUserArray* ua = AS_USERARRAY(args[0]);
    int16_t target_index = ua_normalize_index(ua, AS_NUMBER(args[1]), true);
    if(target_index < 0) {
        return BOOL_VAL(false);
    }
    // These are not sparse arrays, so we'll absolutely have a given index
    // if we know our size is above it.
    return BOOL_VAL( target_index < ua->inner.count );
}


Value cc_function_ar_get(int arg_count, Value* args) {
    if(arg_count != 2 || !IS_USERARRAY(args[0]) || !IS_NUMBER(args[1])) {
        return NIL_VAL;
    }

    ObjUserArray* ua = AS_USERARRAY(args[0]);
    int16_t target_index = ua_normalize_index(ua, AS_NUMBER(args[1]), true);
    if(target_index < 0) {
        return NIL_VAL;
    }

    return ua->inner.values[target_index];
}


Value cc_function_ar_remove(int arg_count, Value* args) {
    if(arg_count != 2 || !IS_USERARRAY(args[0]) || !IS_NUMBER(args[1])) {
        return NIL_VAL;
    }

    ObjUserArray* ua = AS_USERARRAY(args[0]);
    int count = ua->inner.count;
    int16_t target_index = ua_normalize_index(ua, AS_NUMBER(args[1]), true);
    if(target_index < 0) {
        return BOOL_VAL(false);
    }

    for(int i = target_index; i < (count - 1); i++) {
        int j = i + 1;
        ua->inner.values[i] = ua->inner.values[j];
    }
    ua->inner.values[ count - 1 ] = NIL_VAL;
    ua->inner.count--;
    return BOOL_VAL(true);
}


Value cc_function_ar_count(int arg_count, Value* args) {
    if(arg_count != 1 || !IS_USERARRAY(args[0])) {
        return NIL_VAL;
    }
    return NUMBER_VAL(AS_USERARRAY(args[0])->inner.count);
}


Value cc_function_ar_clear(int arg_count, Value* args) {
    if(arg_count != 1 || !IS_USERARRAY(args[0])) {
        return NIL_VAL;
    }
    ObjUserArray* ua = AS_USERARRAY(args[0]);
    freeValueArray(&ua->inner);
    initValueArray(&ua->inner);
    return BOOL_VAL(true);
}


Value cc_function_ar_push(int arg_count, Value* args) {
    if(arg_count != 2 || !IS_USERARRAY(args[0])) {
        return NIL_VAL;
    }

    ObjUserArray* ua = AS_USERARRAY(args[0]);
    int16_t target_index = ua->inner.count;
    if(target_index >= ua->inner.capacity) {
        ua_grow(ua, target_index + 1);
    }
    ua->inner.values[target_index] = args[1];
    ua->inner.count++;
    return NUMBER_VAL(ua->inner.count);
}


Value cc_function_ar_unshift(int arg_count, Value* args) {
    if(arg_count != 2 || !IS_USERARRAY(args[0])) {
        return NIL_VAL;
    }

    ObjUserArray* ua = AS_USERARRAY(args[0]);
    // We're moving all of the elements down by one, then setting the passed
    // value as index zero.  Let's make sure we have room to grow.
    ua->inner.count++;
    if(ua->inner.count >= ua->inner.capacity) {
        ua_grow(ua, ua->inner.count + 1);
    }
    for(int i = ua->inner.count; i > 0; i--) {
        int j = i - 1;
        ua->inner.values[i] = ua->inner.values[j];
    }
    ua->inner.values[0] = args[1];
    return NUMBER_VAL(ua->inner.count);
}


Value cc_function_ar_pop(int arg_count, Value* args) {
    if(arg_count != 1 || !IS_USERARRAY(args[0])) {
        return NIL_VAL;
    }

    ObjUserArray* ua = AS_USERARRAY(args[0]);

    Value old_value = ua->inner.values[ua->inner.count - 1];
    ua->inner.values[ua->inner.count - 1] = NIL_VAL;
    ua->inner.count--;
    return old_value;
}


Value cc_function_ar_shift(int arg_count, Value* args) {
    if(arg_count != 1 || !IS_USERARRAY(args[0])) {
        return NIL_VAL;
    }

    ObjUserArray* ua = AS_USERARRAY(args[0]);

    Value old_value = ua->inner.values[0];
    for(int i = 1; i < ua->inner.count; i++) {
        int j = i - 1;
        ua->inner.values[j] = ua->inner.values[i];
    }
    ua->inner.values[ua->inner.count - 1] = NIL_VAL;
    ua->inner.count--;
    return old_value;
}


Value cc_function_ar_clone(int arg_count, Value* args) {
    if(arg_count != 1 || !IS_USERARRAY(args[0])) {
        return NIL_VAL;
    }

    ObjUserArray* ua = AS_USERARRAY(args[0]);
    ObjUserArray* new_ua = newUserArray();
    ua_grow(new_ua, ua->inner.count);
    for(int i = 0; i < ua->inner.count; i++) {
        new_ua->inner.values[i] = ua->inner.values[i];
    }
    new_ua->inner.count = ua->inner.count;
    return OBJ_VAL(new_ua);
}


Value cc_function_ar_find(int arg_count, Value* args) {
    if(arg_count < 2 || arg_count > 3 || !IS_USERARRAY(args[0])) {
        return NIL_VAL;
    }

    ObjUserArray* ua = AS_USERARRAY(args[0]);
    Value target_value = args[1];
    int16_t minimum_index = 0;
    if(arg_count == 3 && IS_NUMBER(args[2])) {
        minimum_index = ua_normalize_index(ua, AS_NUMBER(args[2]), true);
        if(minimum_index < 0) {
            return NIL_VAL;
        }
    }

    for(int i = minimum_index; i < ua->inner.count; i++) {
        if(valuesEqual(target_value, ua->inner.values[i])) {
            return NUMBER_VAL(i);
        }
    }
    return BOOL_VAL(false);
}


Value cc_function_ar_contains(int arg_count, Value* args) {
    Value find_result = cc_function_ar_find(arg_count, args);
    if(IS_NIL(find_result)) {
        return NIL_VAL;
    }
    if(IS_NUMBER(find_result)) {
        return BOOL_VAL(true);
    }
    return BOOL_VAL(false);
}


Value cc_function_ar_chunk(int arg_count, Value* args) {
    if(arg_count < 2 || !IS_USERARRAY(args[0]) || !IS_NUMBER(args[1])) {
        return NIL_VAL;
    }
    ObjUserArray* ua = AS_USERARRAY(args[0]);

    int16_t chunk_size = (int16_t)AS_NUMBER(args[1]);
    if(chunk_size < 1) {
        return NIL_VAL;
    }

    // Our task is to break our current array into a set of arrays no more than
    // chunk_size long.  We'll start by creating a new User Array with enough
    // capacity to hold all of the child arrays.
    ObjUserArray* result_array = newUserArray();
    ua_grow(result_array, (int16_t)ceil( (double)ua->inner.count / (double)chunk_size ));

    int chunk_counter = 0;
    int result_index = 0;
    result_array->inner.values[result_index] = OBJ_VAL(newUserArray());
    result_array->inner.count = 1;
    for(int i = 0; i < ua->inner.count; i++) {
        // Each chunk is always a maximum size, so we can adjust it immediately.
        ObjUserArray* target_array = AS_USERARRAY(result_array->inner.values[result_index]);
        if(target_array->inner.capacity < chunk_size) {
            ua_grow(target_array, chunk_size);
        }

        target_array->inner.values[chunk_counter] = ua->inner.values[i];
        target_array->inner.count++;

        chunk_counter++;
        if(chunk_counter == chunk_size) {
            result_index++;
            result_array->inner.values[result_index] = OBJ_VAL(newUserArray());
            result_array->inner.count++;
            chunk_counter = 0;
        }
    }
    // We might end up with one too many arrays.  Make sure the last isn't empty.
    ObjUserArray* last_subarray = AS_USERARRAY(
        result_array->inner.values[ result_array->inner.count - 1 ]
    );
    if(last_subarray->inner.count == 0) {
        result_array->inner.count--;
        result_array->inner.values[ result_array->inner.count ] = NIL_VAL;
    }
    return OBJ_VAL(result_array);
}


Value cc_function_ar_shuffle(int arg_count, Value* args) {
    if(arg_count != 1 || !IS_USERARRAY(args[0])) {
        return NIL_VAL;
    }

    ObjUserArray* ua = AS_USERARRAY(args[0]);
    ObjUserArray* target_array = newUserArray();
    ua_grow(target_array, ua->inner.count);

    for(int i = 0; i < ua->inner.count; i++) {
        target_array->inner.values[i] = ua->inner.values[i];
        target_array->inner.count++;
    }
    for(int i = 0; i < target_array->inner.count; i++) {
        Value old_value = target_array->inner.values[i];
        int swap_index = (int)random_int(0, target_array->inner.count - 1);
        target_array->inner.values[i] = target_array->inner.values[swap_index];
        target_array->inner.values[swap_index] = old_value;
    }
    return OBJ_VAL(target_array);
}


Value cc_function_ar_reverse(int arg_count, Value* args) {}
Value cc_function_ar_sort(int arg_count, Value* args) {}
Value cc_function_ar_slice(int arg_count, Value* args) {}
Value cc_function_ar_splice(int arg_count, Value* args) {}

void cc_register_ext_userarray() {

    defineNative("val_is_userarray" ,cc_function_val_is_userarray);

    defineNative("ar_create",     cc_function_ar_create);

    defineNative("ar_set",        cc_function_ar_set);
    defineNative("ar_update",     cc_function_ar_update);
    defineNative("ar_has",        cc_function_ar_has);
    defineNative("ar_get",        cc_function_ar_get);
    defineNative("ar_remove",     cc_function_ar_remove);
    defineNative("ar_count",      cc_function_ar_count);
    defineNative("ar_clear",      cc_function_ar_clear);

    defineNative("ar_push",       cc_function_ar_push);
    defineNative("ar_unshift",    cc_function_ar_unshift);
    defineNative("ar_pop",        cc_function_ar_pop);
    defineNative("ar_shift",      cc_function_ar_shift);

    defineNative("ar_clone",      cc_function_ar_clone);

    defineNative("ar_find",       cc_function_ar_find);
    defineNative("ar_contains",   cc_function_ar_contains);

    defineNative("ar_chunk",      cc_function_ar_chunk);
    defineNative("ar_shuffle",    cc_function_ar_shuffle);
    defineNative("ar_reverse",    cc_function_ar_reverse);

    defineNative("ar_sort",       cc_function_ar_sort);

    defineNative("ar_slice",      cc_function_ar_slice);
    defineNative("ar_splice",     cc_function_ar_splice);
}
