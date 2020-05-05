#include <stdio.h>
#include <math.h>
#include <string.h>

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


Value cc_function_ar_reverse(int arg_count, Value* args) {
    if(arg_count != 1 || !IS_USERARRAY(args[0])) {
        return NIL_VAL;
    }

    ObjUserArray* ua = AS_USERARRAY(args[0]);
    ObjUserArray* target_array = newUserArray();
    ua_grow(target_array, ua->inner.count);

    for(int i = 0; i < ua->inner.count; i++) {
        target_array->inner.values[ ua->inner.count - 1 - i ] = ua->inner.values[i];
        target_array->inner.count++;
    }
    return OBJ_VAL(target_array);
}


Value cc_function_ar_slice(int arg_count, Value* args) {
    if(arg_count < 2 || !IS_USERARRAY(args[0]) || !IS_NUMBER(args[1])) {
        return NIL_VAL;
    }

    ObjUserArray* ua = AS_USERARRAY(args[0]);
    int16_t target_index = ua_normalize_index(ua, AS_NUMBER(args[1]), true);
    if(target_index < 0) {
        return NIL_VAL;
    }

    int max_length = ua->inner.count - target_index;
    int length = max_length;
    if(arg_count == 3 && IS_NUMBER(args[2])) {
        length = (int)AS_NUMBER(args[2]);
    }
    if(length > max_length) {
        length = max_length;
    }
    // It's possible that we're going to be given a negative length.  We'll copy
    // the behavior of string_length() here by shifting around the target index
    // in order to get the right window below.
    if(length < 0) {
        length = length * -1;
        target_index -= length;
        if(target_index < 0) {
            target_index += ua->inner.count;
        }
    }

    ObjUserArray* new_ua = newUserArray();
    ua_grow(new_ua, length);
    for(int i = target_index; i < target_index + length && i < ua->inner.count; i++) {
        new_ua->inner.values[ new_ua->inner.count++ ] = ua->inner.values[i];
    }

    // It's possible that we'll just have created an empty array, so let's not.
    if(new_ua->inner.count == 0) {
        return NIL_VAL;
    }

    return OBJ_VAL(new_ua);
}


Value cc_function_ar_splice(int arg_count, Value* args) {}


static int8_t sort_value_pair(Value example, Value specimen) {
    // Let's get the easy case out of the way first.
    if(valuesEqual(example, specimen)) {
        return 0;
    }
    // This sorting operation works by comparing an example Value against a
    // different specimen Value.  We will return 1 if the specimen should be
    // sorted higher than the example.  We will return -1 if the specimen
    // should be sorted lower than the example.  We will return 0 if the order
    // of the specimen should not be changed compared to the example.
    // This code only runs if the example and the specimen are *not* equal.
    if(example.type == specimen.type) {
        switch(example.type) {
            case VAL_BOOL:
                // We are both booleans, but our values are not equal.
                // If this specimen is true, I must be false, so the specimen
                // gets sorted higher than me.  If this specimen is false, I
                // must be true, so the specimen gets sorted lower than me.
                return AS_BOOL(specimen) ? 1 : -1;
            case VAL_NUMBER: {
                // The C spec says isnan returns a "non-zero" value for true.
                // Why the hell didn't they put simple bools in from the beinning!?
                if(isnan(AS_NUMBER(example)) != 0) {
                    // I'm NaN.  I get sorted below anything that isn't myself,
                    // with one exception: other NaNs.  Our specimen will always
                    // be sorted higher than us, unless it's also a NaN.
                    return isnan(AS_NUMBER(specimen)) != 0 ? 0 : 1;
                }
                if(isnan(AS_NUMBER(specimen)) != 0) {
                    // Likewise, if I am not NaN and the specimen is, it gets
                    // sorted lower than me.
                    return -1;
                }
                // Other, normal values get compared as the numbers that they are.
                if(AS_NUMBER(specimen) > AS_NUMBER(example)) {
                    return 1;
                } else if(AS_NUMBER(specimen) < AS_NUMBER(example)) {
                    return -1;
                }
                // This should not be reachable after the valuesEqual() call.
                printf("** ERROR** sort_value_pair: Number to number equality (UNREACHABLE)");
                return 0;
            }
            case VAL_OBJ: {
                if(AS_OBJ(example)->type == AS_OBJ(specimen)->type) {
                    // We are both Objs of the same type.  It is possible that
                    // we might be equal.  The valuesEqual() check above operates
                    // on the Obj reference data, not our internal values.
                    switch(AS_OBJ(example)->type) {
                        case OBJ_USERHASH: {
                            // We are both hashes.  For sorting purposes, we will
                            // compare our key counts.  If I have more keys than
                            // the given specimen, it is sorted lower than me,
                            // etc etc.
                            int example_count = AS_USERHASH(example)->table.count;
                            int specimen_count = AS_USERHASH(specimen)->table.count;
                            if(specimen_count > example_count) {
                                return 1;
                            } else if(example_count > specimen_count) {
                                return -1;
                            }
                            return 0;
                        }
                        case OBJ_USERARRAY: {
                            // Like user hashes, arrays are sorted by key count.
                            int example_count = AS_USERARRAY(example)->inner.count;
                            int specimen_count = AS_USERARRAY(specimen)->inner.count;
                            if(specimen_count > example_count) {
                                return 1;
                            } else if(example_count > specimen_count) {
                                return -1;
                            }
                            return 0;
                        }
                        case OBJ_STRING: {
                            // We're going to fall back to C's normal sorting
                            // behavior for strings.  Who needs Unicode?  Pfah!
                            // strcmp's parameter order is the opposite of ours.
                            int asciibetical_lol = strcmp(
                                AS_CSTRING(specimen),
                                AS_CSTRING(example)
                            );
                            // strcmp returns a value that probably has some
                            // useful computational value, but we only care if
                            // it's positive, negative, or zero.
                            if(asciibetical_lol > 0) {
                                return 1;
                            } else if(asciibetical_lol < 0) {
                                return -1;
                            }
                            return 0;
                        }
                        // All other types are considered equal to themselves.
                        case OBJ_FUNCTION:
                        case OBJ_NATIVE: {
                            return 0;
                        }
                        default: {
                            printf("** ERROR** sort_value_pair: Objs type equal fallthrough (UNREACHABLE)");
                            return 0;
                        }
                    }
                }
                // We are both Objs, but we are of different types.  There is no
                // sane mechanism for value comparison between Obj types.
                // Instead of bailing out, we'll compare our Obj types themselves.
                // The sorting order for Obj types is as follows, from low:
                // Others < Natives < Functions < Hashes < Arrays < Strings
                switch(AS_OBJ(example)->type) {
                    case OBJ_STRING:
                        // I am a string.  Everything else is below me.
                        return -1;
                    case OBJ_USERARRAY: {
                        if(AS_OBJ(specimen)->type == OBJ_STRING) {
                            return 1;
                        }
                        return -1;
                    }
                    case OBJ_USERHASH: {
                        switch(AS_OBJ(specimen)->type) {
                            case OBJ_STRING:
                            case OBJ_USERARRAY:
                                return 1;
                            default:
                                return -1;
                        }
                    }
                    case OBJ_FUNCTION: {
                        switch(AS_OBJ(specimen)->type) {
                            case OBJ_STRING:
                            case OBJ_USERARRAY:
                            case OBJ_USERHASH:
                                return 1;
                            default:
                                return -1;
                        }
                    }
                    case OBJ_NATIVE:
                        // I'm a native, but the specimen is not.  It always gets
                        // sorted higher than me.  Gettin' all colonial here.
                        return 1;
                    default: {
                        printf("** ERROR** sort_value_pair: Objs type diff fallthrough (UNREACHABLE)");
                        return 0;
                    }
                }
            }
            default: {
                printf("** ERROR** sort_value_pair: Values type equal fallthrough (UNREACHABLE)");
                return 0;
            }
        }
    }
    // Our types are not equal.  We will compare ourself with the specimen based
    // on our types in the following order: NaN, nil, bool, non-NaN numbers,
    // then Objs.
    if(IS_NUMBER(specimen) && isnan(AS_NUMBER(specimen)) != 0) {
        // The specimen is NaN, so it should always get sorted lower than me.
        // We know this because this code won't run if I'm also a number.
        return -1;
    }
    switch(example.type) {
        case VAL_NIL: {
            // I am nil, and the specimen is neither nil nor NaN. It gets sorted
            // higher than us.
            return 1;
        }
        case VAL_BOOL: {
            // I'm a boolean, so the specimen will be sorted higher than me
            // unless it's a nil.  The specimen can not be NaN.
            return IS_NIL(specimen) ? -1 : 1;
        }
        case VAL_NUMBER: {
            if(isnan(AS_NUMBER(example)) != 0) {
                // Oh no, I'm NaN!  The specimen can't be NaN at this point.
                return 1;
            }
            // I'm a non-NaN number.  Only Objs are sorted higher than us.
            return IS_OBJ(specimen) ? 1 : -1;
        }
        case VAL_OBJ: {
            // I'm an Obj, and the specimen is not.  It is sorted below me.
            return -1;
        }
        default: {
            printf("** ERROR** sort_value_pair: Values type diff fallthrough (UNREACHABLE)");
            return 0;
        }
    }
    printf("** ERROR** sort_value_pair: Complete fallthrough! (UNREACHABLE)");
    return 0;
}

void sort_bruteforce(int count, Value* values) {
    bool is_sorted = false;
    int rounds = 0;
    int swap_count = 0;
    while(is_sorted == false) {
        is_sorted = true;
        for(int i = 0; i < count - 1; i++) {
            int j = i + 1;
            Value left = values[i];
            Value right = values[j];
            if(sort_value_pair(left, right) == -1) {
                // The comparison routine says that the right Value is sorted
                // less than the left Value.  Swap them.
                swap_count++;
                is_sorted = false;
                values[i] = right;
                values[j] = left;
            }
        }
        // printf("** sort_bruteforce round %d: %d swaps\n", rounds++, swap_count);
        swap_count = 0;
    }
}


Value cc_function_ar_sort(int arg_count, Value* args) {
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

    sort_bruteforce(target_array->inner.count, target_array->inner.values);

    return OBJ_VAL(target_array);
}


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
