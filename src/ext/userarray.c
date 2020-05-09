#include <stdio.h>
#include <math.h>
#include <string.h>

#include "../memory.h"
#include "../object.h"
#include "../value.h"
#include "../vm.h"

#include "number.h"


void ua_grow(ObjUserArray* ua, int new_capacity) {
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
    int64_t index = (int64_t)floor(target_index);
    if(index < 0) {
        index += ua->inner.count;
    }

    if(index > INT16_MAX || index < 0 || (valid_indexes_only && index >= ua->inner.count)) {
        return -1;
    }
    return (int16_t)index;
}

static int16_t ua_normalize_range(ObjUserArray* ua, uint16_t target_index, double target_range) {
    int64_t range = (int64_t)floor(target_range);
    if (range + target_index < 0) {
        // Only allow negative ranges to reach the first element in the array.
        // That is, ranges can't go negative and loop around like an index.
        range = target_index * -1;
    }
    if(range + target_index >= ua->inner.count) {
        // Clamp positive ranges to the end of the array.
        range = ua->inner.count - target_index;
    }
    return (int16_t)range;
}

struct UA_Legal_Range {
    int16_t index;
    int16_t range;
    bool error;
};

static struct UA_Legal_Range ua_normalize_index_range(
    ObjUserArray* ua, double target_index, double target_range
  ) {
      struct UA_Legal_Range legal;
      legal.error = false;
      legal.index = ua_normalize_index(ua, target_index, true);
      if(legal.index < 0) {
          legal.error = true;
          legal.index = 0;
      }
      legal.range = ua_normalize_range(ua, legal.index, target_range);
      if(legal.range < 0 && !legal.error) {
          legal.range = legal.range * -1;
          legal.index -= legal.range;
          legal.range++;
      }
      return legal;
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


/**
 * ar_set(user_array, key, value)
 * - returns nil on parameter error
 * - returns false if the given index is negative and out of legal range
 * - returns true on success
 */
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


/**
 * ar_update(user_array, key, value)
 * - returns nil on paramater error
 * - returns false if the given index is negative and out of legal range
 * - returns the previous value of the given key on success, which may be nil
 */
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

/**
 * ar_has(user_array, key)
 * - returns nil on paramater error
 * - returns false if the the given index is out of bounds of the array
 * - returns true otherwise
 */
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


/**
 * ar_get(user_array, key)
 * - returns nil on paramater error
 * - returns nil if the the given index is out of bounds of the array
 *   !! Normally key-checking functions like this would instead return false.
 *      This function returns nil instead of false to remove possible ambiguity
 *      over the actual value being stored.  @FIXME This language needs errors!
 * - returns the value of the given key on success, which may still be nil.
 */
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

/**
 * ar_remove(user_array, key, count? = 1)
 * - returns nil on parameter error
 * - returns false if the the given index is out of bounds of the array
 * - returns the numeric count of the number of values removed from the array,
 *   after the key and count are normalized to fit within the bounds of the array.
 */
Value cc_function_ar_remove(int arg_count, Value* args) {
    if(arg_count < 2 || arg_count > 4 || !IS_USERARRAY(args[0]) || !IS_NUMBER(args[1])) {
        return NIL_VAL;
    }

    ObjUserArray* ua = AS_USERARRAY(args[0]);
    double raw_range = 1;
    if(arg_count == 3 && IS_NUMBER(args[2])) {
        raw_range = AS_NUMBER(args[2]);
    }
    struct UA_Legal_Range legal = ua_normalize_index_range(ua, AS_NUMBER(args[1]), raw_range);
    if(legal.error) {
        return BOOL_VAL(false);
    }

    int starting_index = legal.index;
    int distance = legal.range;
    int max_index = ua->inner.count - 1 - distance;

    if(distance == 0) {
        return BOOL_VAL(false);
    }

    // We're creating a gap, and then backfilling it with elements from the rest
    // of the array, only we're actually skipping the gap creation.
    for(int i = starting_index; i <= max_index; i++) {
        ua->inner.values[i] = ua->inner.values[i + distance];
    }
    // We've copied the values over, not moved them.  Clear up the old copies.
    for(int i = ua->inner.count - 1; i > max_index; i--) {
        ua->inner.values[i] = NIL_VAL;
    }
    ua->inner.count -= distance;
    return NUMBER_VAL(distance);
}

/**
 * ar_count(user_array)
 * - returns nil on parameter error
 * - returns the number of entries in the array, which may be zero
 */
Value cc_function_ar_count(int arg_count, Value* args) {
    if(arg_count != 1 || !IS_USERARRAY(args[0])) {
        return NIL_VAL;
    }
    return NUMBER_VAL(AS_USERARRAY(args[0])->inner.count);
}


/**
 * ar_clear(user_array)
 * - returns nil on parameter error
 * - returns true on success
 */
Value cc_function_ar_clear(int arg_count, Value* args) {
    if(arg_count != 1 || !IS_USERARRAY(args[0])) {
        return NIL_VAL;
    }
    ObjUserArray* ua = AS_USERARRAY(args[0]);
    freeValueArray(&ua->inner);
    initValueArray(&ua->inner);
    return BOOL_VAL(true);
}


/**
 * ar_push(user_array, value)
 * - returns nil on parameter error
 * - returns the new number of elements in the array
 */
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


/**
 * ar_unshift(user_array, value)
 * - returns nil on parameter error
 * - returns the new number of elements in the array
 */
Value cc_function_ar_unshift(int arg_count, Value* args) {
    if(arg_count != 2 || !IS_USERARRAY(args[0])) {
        return NIL_VAL;
    }

    ObjUserArray* ua = AS_USERARRAY(args[0]);
    // We're moving all of the elements up by one, then setting the passed
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


/**
 * ar_pop(user_array)
 * - returns nil on parameter error
 * - returns the value of the last element of the array, after it has been removed
 */
Value cc_function_ar_pop(int arg_count, Value* args) {
    if(arg_count != 1 || !IS_USERARRAY(args[0])) {
        return NIL_VAL;
    }

    ObjUserArray* ua = AS_USERARRAY(args[0]);

    if(ua->inner.count == 0) {
        return NIL_VAL;
    }

    Value old_value = ua->inner.values[ua->inner.count - 1];
    ua->inner.values[ua->inner.count - 1] = NIL_VAL;
    ua->inner.count--;
    return old_value;
}


/**
 * ar_shift(user_array)
 * - returns nil on parameter error
 * - returns the value of the first element in the array, after it has been removed
 */
Value cc_function_ar_shift(int arg_count, Value* args) {
    if(arg_count != 1 || !IS_USERARRAY(args[0])) {
        return NIL_VAL;
    }

    ObjUserArray* ua = AS_USERARRAY(args[0]);

    if(ua->inner.count == 0) {
        return NIL_VAL;
    }

    Value old_value = ua->inner.values[0];
    for(int i = 1; i < ua->inner.count; i++) {
        int j = i - 1;
        ua->inner.values[j] = ua->inner.values[i];
    }
    ua->inner.values[ua->inner.count - 1] = NIL_VAL;
    ua->inner.count--;
    return old_value;
}


/**
 * ar_clone(user_array)
 * - returns nil on parameter error
 * - returns a new identical copy of the original array
 */
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


/**
 * ar_find(user_array, value, starting_index?)
 * - returns nil on parameter error
 * - returns nil if the the given starting index is out of bounds of the array
 *   !! Again, a variation here, normally this returns false, but we return
 *      nil here to allow a distingiusihing between bad params and not finding
 *      the desired value.
 * - returns false if the given value is not in the array
 * - returns the numeric index of first location of the value in the array
 *   after the starting index, which is the start of the array unless specified.
 */
Value cc_function_ar_find(int arg_count, Value* args) {
    if(arg_count < 2 || arg_count > 3 || !IS_USERARRAY(args[0])) {
        return NIL_VAL;
    }

    ObjUserArray* ua = AS_USERARRAY(args[0]);
    if(ua->inner.count == 0) {
        return BOOL_VAL(false);
    }

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


/**
 * ar_chunk(user_array, chunk_size)
 * - returns nil on parameter error
 * - returns an array composed of arrays of chunk_size, each holding values taken
 *   from the source array, in the original order.
 */
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
    int16_t outer_capacity = (int16_t)ceil( (double)ua->inner.count / (double)chunk_size );
    ua_grow(result_array, outer_capacity);

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
        if(chunk_counter == chunk_size && i + 1 < ua->inner.count) {
            result_index++;
            result_array->inner.values[result_index] = OBJ_VAL(newUserArray());
            result_array->inner.count++;
            chunk_counter = 0;
        }
    }
    return OBJ_VAL(result_array);
}


/**
 * ar_shuffle(user_array)
 * - returns nil on parameter error
 * - returns a copy of the original array with the values shuffled around by RNG
 */
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
        int swap_index = (int)random_int(0, target_array->inner.count - 1);
        Value old_value = target_array->inner.values[i];
        target_array->inner.values[i] = target_array->inner.values[swap_index];
        target_array->inner.values[swap_index] = old_value;
    }
    return OBJ_VAL(target_array);
}


/**
 * ar_reverse(user_array)
 * - returns nil on parameter error
 * - returns a copy of the original array with the values reversed
 */
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


/**
 * ar_slice(user_array, index, count?)
 * - returns nil on parameter error
 * - returns false if the the given index is out of bounds of the array
 * - returns a new array copied from the existing array, starting at the passed
 *   index.  If a count is passed, only that many elements are copied.  If no
 *   count is passed, all remaining array elements are copied.
 */
Value cc_function_ar_slice(int arg_count, Value* args) {
    if(arg_count < 2 || arg_count > 3 || !IS_USERARRAY(args[0]) || !IS_NUMBER(args[1])) {
        return NIL_VAL;
    }

    ObjUserArray* ua = AS_USERARRAY(args[0]);
    double raw_range = 0;
    if(arg_count == 3 && IS_NUMBER(args[2])) {
        raw_range = AS_NUMBER(args[2]);
    }
    if(raw_range == 0) {
        raw_range = ua->inner.count;
    }
    struct UA_Legal_Range legal = ua_normalize_index_range(ua, AS_NUMBER(args[1]), raw_range);
    if(legal.error) {
        return BOOL_VAL(false);
    }

    ObjUserArray* new_ua = newUserArray();
    ua_grow(new_ua, legal.range);
    for(int i = legal.index; i < legal.index + legal.range; i++) {
        new_ua->inner.values[ new_ua->inner.count++ ] = ua->inner.values[i];
    }
    return OBJ_VAL(new_ua);
}


/**
 * ar_insert(user_array, index, donor_array)
 * - returns nil on parameter error
 * - returns false if the the given index is out of bounds of the array
 * - returns a new array with the contents of the donor array inserted into the
 *   user array, starting at the given idex of the user array.  If the index is
 *   larger than the user array, nils are inserted before inserting the donor.
 */
Value cc_function_ar_insert(int arg_count, Value* args) {
    if(arg_count < 3 || !IS_USERARRAY(args[0]) || !IS_NUMBER(args[1]) || !IS_USERARRAY(args[2])) {
        return NIL_VAL;
    }

    ObjUserArray* left = AS_USERARRAY(args[0]);
    ObjUserArray* right = AS_USERARRAY(args[2]);
    int16_t target_index = ua_normalize_index(left, AS_NUMBER(args[1]), false);
    if(target_index < 0) {
        return BOOL_VAL(false);
    }

    // We aren't doing bounds checking on the insert index.  If it's geater than
    // our size, we will need to insert blanks at the end of the section to the
    // left of the insert.
    int addtl = target_index - left->inner.count;
    if(addtl < 0) {
        addtl = 0;
    }

    ObjUserArray* new_ua = newUserArray();
    ua_grow(new_ua, left->inner.count + addtl + right->inner.count);

    // We can stop anywhere inside the first array, or even outside of its bounds.
    // Make sure that we don't go out of index during the first step.
    int first_stop = addtl > 0 ? left->inner.count : target_index;

    // Step 1: Copy from the left array up to the bounary.
    for(int i = 0; i < first_stop; i++) {
        new_ua->inner.values[ new_ua->inner.count++ ] = left->inner.values[i];
    }
    // Step 2: Insert blanks to hit the target index
    if(addtl > 0) {
        for(int i = 0; i < addtl; i++) {
            new_ua->inner.values[ new_ua->inner.count++ ] = NIL_VAL;
        }
    }
    // Step 3: Insert the right array
    for(int i = 0; i < right->inner.count; i++) {
        new_ua->inner.values[ new_ua->inner.count++ ] = right->inner.values[i];
    }
    // Step 4: Finish copying from the left array
    for(int i = first_stop; i < left->inner.count; i++) {
        new_ua->inner.values[ new_ua->inner.count++ ] = left->inner.values[i];
    }

    return OBJ_VAL(new_ua);
}


/**
 * ar_append(user_array, donor_array)
 * - returns nil on parameter error
 * - returns a copy of the original with the elements from donor array appended
 */
Value cc_function_ar_append(int arg_count, Value* args) {
    if(arg_count != 2 || !IS_USERARRAY(args[0]) || !IS_USERARRAY(args[1])) {
        return NIL_VAL;
    }

    ObjUserArray* left = AS_USERARRAY(args[0]);
    ObjUserArray* right = AS_USERARRAY(args[1]);
    ObjUserArray* new_ua = newUserArray();
    ua_grow(new_ua, left->inner.count + right->inner.count);
    for(int i = 0; i < left->inner.count; i++) {
        new_ua->inner.values[ new_ua->inner.count++ ] = left->inner.values[i];
    }
    for(int i = 0; i < right->inner.count; i++) {
        new_ua->inner.values[ new_ua->inner.count++ ] = right->inner.values[i];
    }
    return OBJ_VAL(new_ua);
}


/**
 * ar_prepend(user_array, donor_array)
 * - returns nil on parameter error
 * - returns a copy of the original with the elements from donor array prepended
 */
Value cc_function_ar_prepend(int arg_count, Value* args) {
    if(arg_count != 2 || !IS_USERARRAY(args[0]) || !IS_USERARRAY(args[1])) {
        return NIL_VAL;
    }

    // lol
    Value swap = args[0];
    args[0] = args[1];
    args[1] = swap;
    Value res = cc_function_ar_append(arg_count, args);
    // So these are pointers to things that are on the stack.  Undo our damage.
    swap = args[0];
    args[0] = args[1];
    args[1] = swap;
    return res;
}


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


static void quicksort_recursive(int min_index, int max_index, Value* values) {
    // Don't attempt to sort lists of size 0 or 1.
    if(max_index - max_index < 2) {
        return;
    }
/*
    An explanation of quicksort based on:
    https://www.khanacademy.org/computing/computer-science/algorithms/quick-sort/a/overview-of-quicksort
    https://www.khanacademy.org/computing/computer-science/algorithms/quick-sort/a/linear-time-partitioning

    Those documents describe keeping track of multiple indexes for the various
    parts of the array, but as you will see, we really only need to keep track
    of the gt index.  Everything else can be derived from the passed min and
    max indexes, given that we always pivot on the final index.  (The lte index
    is always min_index!)

    We are given an array of Values, and a range to operate within that array.
    We select the last element of the array.  This is our pivot value.  We will
    partition the array into two sections: those less than or equal to the pivot,
    and those that are larger than the pivot.

    We will keep track of these partitions by keeping two indexes:
    1) The index of the start of the lte values, initially 0
    2) The index of the start of the gt values, initially 0

    We will iterate through the array and compare each value to the pivot.

    Here's our sample array:
    0     1     2     3     4     5     6     7
    [15]  [17]  [11]  [12]  [19]  [11]  [13]  [14]
    (lte)
    (gt)

    Our pivot is index 7, value 14.

    Start by comparing element 0 to our pivot.  It's greater than the pivot.  The
    gt index is checked.  Because we are inside the gt range, we will do nothing.

    Element 1 is next.  Again because it's greater than the pivot, and we're inside
    the gt range, we will do nothing.

    Element 2 is next.  It is lte to the pivot.  We will swap it with the first
    index of the gt list (element 0), and then increment the gt index by one.
    The array now looks like:

    0     1     2     3     4     5     6     7
    [11]  [17]  [15]  [12]  [19]  [11]  [13]  [14]
    (lte) (gt)                                (p)

    Element 3 is next.  It is lte to the pivot.  Swap it with the first index of
    the gt list, inc the gt index.

    0     1     2     3     4     5     6     7
    [11]  [12]  [15]  [17]  [19]  [11]  [13]  [14]
    (lte)       (gt)                          (p)

    Element 4 is next.  It is larger and inside the gt range, so we skip.

    Element 5 is next.  It is smaller.  Swap it with the gt index, inc gt.

    0     1     2     3     4     5     6     7
    [11]  [12]  [11]  [17]  [19]  [15]  [13]  [14]
    (lte)             (gt)                    (p)

    Element 6 is smaller, swap with gt, inc gt.

    0     1     2     3     4     5     6     7
    [11]  [12]  [11]  [13]  [19]  [15]  [17]  [14]
    (lte)                   (gt)              (p)

    We're done with the first step.  Swap the gt index and the pivot, then inc gt.

    0     1     2     3     4     5     6     7
    [11]  [12]  [11]  [13]  [14]  [15]  [17]  [19]
    (lte)                   (p)   (gt)

    Recursion time.  Sort the left half of the array, from the min index through
    to the pivot.  Then sort the right half of the array, from the gt index
    through to max index.
*/
    int pivot_index = max_index;
    int gt_index = min_index;
    Value pivot = values[pivot_index];
    for(int i = min_index; i < pivot_index; i++) {
        Value specimen = values[i];
        if(sort_value_pair(pivot, specimen) < 1) {
        // The specimen value is sorted lower than or equal to the pivot.  Move it
        // to the end of the lte section, which is denoted by the gt index.
        // As the lte section grows, the gt index will increment accordingly.
            values[i] = values[gt_index];
            values[gt_index] = specimen;
            gt_index++;
        }
    }
    // We've now split the array into two sections:
    // min_index through gt_index - 1: All values lte to the pivot.
    // gt_index through pivot_index - 1: All values gt to the pivot.
    // The pivot belongs with the gt section, so swap it there now.
    values[pivot_index] = values[gt_index];
    values[gt_index] = pivot;
    // Recursion time.  Call ourselves for the left and right sides of the array.
    if(gt_index - min_index > 1) {
        quicksort_recursive(min_index, gt_index - 1, values);
    }
    if(max_index - gt_index > 1) {
        quicksort_recursive(gt_index, max_index, values);
    }
}


static void quicksort_recursive_callback(int min_index, int max_index, Value* values, Value callback) {
    int pivot_index = max_index;
    int gt_index = min_index;
    Value pivot = values[pivot_index];
    for(int i = min_index; i < pivot_index; i++) {
        Value specimen = values[i];

        Value callback_args[2] = { pivot, specimen };
        Value sort_result = callCallback(callback, 2, callback_args);
        // If we get a non-number result back, nothing really changes here.
        // We're partitioning here based only if the specimen is gt, not lte,
        // and we'll treat any less-than-1 value as lte.
        if(IS_NUMBER(sort_result) && AS_NUMBER(sort_result) < 1) {
            values[i] = values[gt_index];
            values[gt_index] = specimen;
            gt_index++;
        }
    }
    values[pivot_index] = values[gt_index];
    values[gt_index] = pivot;
    if(gt_index - min_index > 1) {
        quicksort_recursive_callback(min_index, gt_index - 1, values, callback);
    }
    if(max_index - gt_index > 1) {
        quicksort_recursive_callback(gt_index, max_index, values, callback);
    }
}


/**
 * ar_sort(array)
 * - returns nil on parameter error
 * - returns a copy of the original array with the elements sorted
 */
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

    quicksort_recursive(0, target_array->inner.count - 1, target_array->inner.values);

    return OBJ_VAL(target_array);
}


/**
 * ar_sort_callback(array, callback)
 * - returns nil on parameter error
 * - returns a copy of the original array with the elements sorted using the
 *   given callback function to compare elements.
 *
 * => callback(example, specimen)
 * - returns -1, 0, or 1 based on whether the specimen should be sorted below,
 *   equal to, or above the example.  Non-numeric return values are treated as
 *   if zero was returned instead.
 */
Value cc_function_ar_sort_callback(int arg_count, Value* args) {
    if(arg_count != 2 || !IS_USERARRAY(args[0]) || !IS_FUNCTION(args[1])) {
        return NIL_VAL;
    }

    ObjUserArray* ua = AS_USERARRAY(args[0]);
    ObjUserArray* target_array = newUserArray();
    ua_grow(target_array, ua->inner.count);

    for(int i = 0; i < ua->inner.count; i++) {
        target_array->inner.values[i] = ua->inner.values[i];
        target_array->inner.count++;
    }

    quicksort_recursive_callback(
        0, target_array->inner.count - 1,
        target_array->inner.values,
        args[1]
    );

    return OBJ_VAL(target_array);
}


/**
 * ar_join(array, glue_string)
 * - returns nil on parameter error
 * - returns false if a non-string, non-nil element is in the array
 *   @FIXME ugh, need a way to force-stringify things
 * - returns a string of all array elements joined together with the glue string
 */
Value cc_function_ar_join(int arg_count, Value* args) {
    if(arg_count != 2 || !IS_USERARRAY(args[0])|| !IS_STRING(args[1])) {
        return NIL_VAL;
    }

    ObjUserArray* ua = AS_USERARRAY(args[0]);
    ObjString* glue = AS_STRING(args[1]);

    int new_string_length = (ua->inner.count - 1) * glue->length;
    if(new_string_length < 0) {
        // We can end up negative if the array is empty.  Let's not.
        new_string_length = 0;
    }
    for(int i = 0; i < ua->inner.count; i++) {
        // We can only join together things that are strings, but we'll be nice
        // and skip nils.  If there's anything we can process, bail now.
        if(!IS_NIL(ua->inner.values[i]) && !IS_STRING(ua->inner.values[i])) {
            return BOOL_VAL(false);
        }
        if(IS_NIL(ua->inner.values[i])) {
            continue;
        }
        new_string_length += AS_STRING(ua->inner.values[i])->length;
    }

    char* new_string = ALLOCATE(char, new_string_length + 1);
    int new_string_index = 0;
    for(int i = 0; i < ua->inner.count; i++) {
        // If we're here, we know that all elements in the array are either
        // strings or nil.  Nil becomes the empty string, so we can ignore it.
        if(IS_STRING(ua->inner.values[i])) {
            ObjString* str = AS_STRING(ua->inner.values[i]);
            // dest, src, length
            memcpy(&new_string[new_string_index], str->chars, str->length);
            new_string_index += str->length;
        }
        // Don't put glue after the final element in the array
        if(i != ua->inner.count - 1) {
            memcpy(&new_string[new_string_index], glue->chars, glue->length);
            new_string_index += glue->length;
        }
    }
    new_string[new_string_length] = '\0';

    return OBJ_VAL(takeString(new_string, new_string_length));
}


/**
 * ar_filter(array, callback)
 * - returns nil on parameter error
 * - returns a copy of the array filtered using the provided callback.
 *
 * => callback(value, index)
 * - returns true if the array element should be included in the copy
 * - returns non-true otherwise
 */
Value cc_function_ar_filter(int arg_count, Value* args) {
    if(arg_count != 2 || !IS_USERARRAY(args[0]) || !IS_FUNCTION(args[1])) {
        return NIL_VAL;
    }

    ObjUserArray* ua = AS_USERARRAY(args[0]);
    ObjFunction* callback = AS_FUNCTION(args[1]);

    ObjUserArray* new_ua = newUserArray();
    ua_grow(new_ua, ua->inner.count);

    for(int i = 0; i < ua->inner.count; i++) {
        Value callback_args[2] = {
            ua->inner.values[i],
            NUMBER_VAL(i)
        };
        Value res = callCallback(OBJ_VAL(callback), 2, callback_args);
        if(IS_BOOL(res) && AS_BOOL(res) == true) {
            new_ua->inner.values[ new_ua->inner.count++ ] = ua->inner.values[i];
        }
    }
    return OBJ_VAL(new_ua);
}


/**
 * ar_map(array, callback)
 * - returns nil on parameter error
 * - returns a copy of the array with each elemnt processed by the provided callback
 *
 * => callback(value, index)
 * - returns the new value that should be placed at the offset
 */
Value cc_function_ar_map(int arg_count, Value* args) {
    if(arg_count != 2 || !IS_USERARRAY(args[0]) || !IS_FUNCTION(args[1])) {
        return NIL_VAL;
    }

    ObjUserArray* ua = AS_USERARRAY(args[0]);
    ObjFunction* callback = AS_FUNCTION(args[1]);

    ObjUserArray* new_ua = newUserArray();
    ua_grow(new_ua, ua->inner.count);

    for(int i = 0; i < ua->inner.count; i++) {
        Value callback_args[2] = {
            ua->inner.values[i],
            NUMBER_VAL(i)
        };
        Value res = callCallback(OBJ_VAL(callback), 2, callback_args);
        new_ua->inner.values[ new_ua->inner.count++ ] = res;
    }
    return OBJ_VAL(new_ua);
}


/**
 * ar_reduce(array, callback)
 * - returns nil on parameter error
 * - returns a single value, the last returned by the callback
 *
 * => callback(accumulator, value, index)
 * - return value is passed as the accumulator to the next callback, or back to
 *   the user if this is the last index.
 */
Value cc_function_ar_reduce(int arg_count, Value* args) {
    if(arg_count != 2 || !IS_USERARRAY(args[0]) || !IS_FUNCTION(args[1])) {
        return NIL_VAL;
    }

    ObjUserArray* ua = AS_USERARRAY(args[0]);
    ObjFunction* callback = AS_FUNCTION(args[1]);

    Value accumulator = NIL_VAL;
    for(int i = 0; i < ua->inner.count; i++) {
        Value callback_args[3] = {
            accumulator,
            ua->inner.values[i],
            NUMBER_VAL(i)
        };
        accumulator = callCallback(OBJ_VAL(callback), 3, callback_args);
    }
    return accumulator;
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

    defineNative("ar_chunk",      cc_function_ar_chunk);
    defineNative("ar_shuffle",    cc_function_ar_shuffle);
    defineNative("ar_reverse",    cc_function_ar_reverse);

    defineNative("ar_slice",      cc_function_ar_slice);
    defineNative("ar_insert",     cc_function_ar_insert);
    defineNative("ar_append",     cc_function_ar_append);
    defineNative("ar_prepend",    cc_function_ar_prepend);

    defineNative("ar_sort",       cc_function_ar_sort);
    defineNative("ar_sort_callback", cc_function_ar_sort_callback);

    defineNative("ar_join",       cc_function_ar_join);

    defineNative("ar_filter",     cc_function_ar_filter);
    defineNative("ar_map",        cc_function_ar_map);
    defineNative("ar_reduce",     cc_function_ar_reduce);

}
