#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <sys/time.h>
#include <math.h>
#include <fenv.h>

#include "common.h"
#include "compiler.h"
#include "debug.h"
#include "object.h"
#include "memory.h"
#include "table.h"
#include "vm.h"


Value cc_function_string_substring(int arg_count, Value* args) {
  if(arg_count < 1) {
    // Must at least have the string passed through.
    return BOOL_VAL(false);
  }
  if(!IS_STRING(args[0])) {
    // Must at least have *a* string passed through.
    return BOOL_VAL(false);;
  }
  ObjString *source_string = AS_STRING(args[0]);

  int starting_index = 0;
  if(arg_count >= 2 && IS_NUMBER(args[1])) {
    starting_index = (int)AS_NUMBER(args[1]);
    if(abs(starting_index) > source_string->length) {
      // We accept both positive and negative indexes.  If either of them is
      // out of range for the string, we'll assume it's bogus and fail.
      return BOOL_VAL(false);
    }

    if(starting_index < 0) {
      // Given a string length of 8 and a starting index of -5,
      // 8 + (-5) = 3
      // [0]  [1]  [2]  [3]  [4]  [5]  [6]  [7]
      //      -7   -6   -5   -4   -3   -2   -1
      starting_index = source_string->length + starting_index;
    }
  }

  // Given a string length of 8 and a starting index of 3,
  // (8 - 1) - 3 => 7 - 3 => 4 would be the maximum number of chars we can pull from the right.
  // Likewise, the index value is the maximum number of chars we can pull from the left.
  int max_substring_length_right = source_string->length - starting_index;
  int max_substring_length_left = starting_index;
  int substring_length = max_substring_length_right;
  if(arg_count == 3 && IS_NUMBER(args[2])) {
    substring_length = (int)AS_NUMBER(args[2]);

    if(substring_length > max_substring_length_right) {
      // Can't pull more than the string length.
      return BOOL_VAL(false);
    }

    if(substring_length < 0) {
      substring_length = abs(substring_length);
      if(substring_length > max_substring_length_left) {
        // Likewise, we can't pull more than the string length from the left.
        return BOOL_VAL(false);
      }
      // This is pretty silly but I'm doing it anyway.
      starting_index -= substring_length;
    }
  }

  // Could probably do this using memcpy as in concatenate(), but doing manual
  // memory management freaks me out.  I'm sure this is slower, but I'd much
  // rather be explicit and paranoid.
  char* new_string = ALLOCATE(char, substring_length + 1);
  int new_index = 0;
  for(int i = starting_index; i < (starting_index + substring_length); i++) {
    new_string[new_index] = source_string->chars[i];
    new_index++;
  }
  new_string[new_index] = '\0';

  return OBJ_VAL(takeString(new_string, new_index));
}


Value cc_function_string_length(int arg_count, Value* args) {
  if(!IS_STRING(args[0])) {
    return BOOL_VAL(false);
  }
  return NUMBER_VAL( AS_STRING(args[0])->length );
}


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

#ifdef FEATURE_USER_HASHES
Value cc_function_val_is_userhash(int arg_count, Value* args) {
    if(arg_count != 1) {
        return NIL_VAL;
    }
    return BOOL_VAL( IS_USERHASH(args[0]) );
}
#endif


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


Value cc_function_number_absolute(int arg_count, Value* args) {
  if (arg_count != 1 || !IS_NUMBER(args[0])) {
    return NIL_VAL;
  }
  return NUMBER_VAL(fabs(AS_NUMBER(args[0])));
}


Value cc_function_number_remainder(int arg_count, Value* args) {
  if (arg_count != 2 || !IS_NUMBER(args[0]) || !IS_NUMBER(args[1])) {
    return NIL_VAL;
  }
  // Problem: We're trying to get the integer remainder of an integer division,
  // but the data types are doubles that may contain decimal numbers.
  // Solution: trunc() is used to round the value stored in the doubles towards
  // zero while keeping the data type as double.  remainder() operates on
  // doubles.
  double dividend = trunc(AS_NUMBER(args[0]));
  double divisor = trunc(AS_NUMBER(args[1]));
  double result = remainder(dividend, divisor);
  // Sometimes it decides to return a correctish but inappropriately negative
  // number for reasons that I don't understand.  Adjusting it this way makes
  // it "correct."  I'm sure that somehow I'm creating a violation of some
  // mathematical rule by doing this, but it all checks out when it do it by
  // hand on paper.
  if (result < 0) {
    result += divisor;
  }
  return NUMBER_VAL(result);
}


Value cc_function_number_minimum(int arg_count, Value* args) {
  if (arg_count < 1) {
    return NIL_VAL;
  }
  double minimum_value = INFINITY;
  for (int i = 0; i < arg_count; i++) {
    if (IS_NUMBER(args[i]) && AS_NUMBER(args[i]) < minimum_value) {
      minimum_value = AS_NUMBER(args[i]);
    }
  }
  return NUMBER_VAL(minimum_value);
}


Value cc_function_number_maximum(int arg_count, Value* args) {
  if (arg_count < 1) {
    return NIL_VAL;
  }
  double maximum_value = -INFINITY;
  for (int i = 0; i < arg_count; i++) {
    if (IS_NUMBER(args[i]) && AS_NUMBER(args[i]) > maximum_value) {
      maximum_value = AS_NUMBER(args[i]);
    }
  }
  return NUMBER_VAL(maximum_value);
}


Value cc_function_number_floor(int arg_count, Value* args) {
  if (arg_count != 1 || !IS_NUMBER(args[0])) {
    return NIL_VAL;
  }
  return NUMBER_VAL(floor(AS_NUMBER(args[0])));
}


Value cc_function_number_ceiling(int arg_count, Value* args) {
  if (arg_count != 1 || !IS_NUMBER(args[0])) {
    return NIL_VAL;
  }
  return NUMBER_VAL(ceil(AS_NUMBER(args[0])));
}


Value cc_function_number_round(int arg_count, Value* args) {
  if (arg_count < 1 || !IS_NUMBER(args[0]) || arg_count > 2) {
    return NIL_VAL;
  }
  // We'll optionally be passed a precision argument, which may be a positive
  // or negative integer.  Our stdlib functions only operate on integers.
  // We can simulate the requested precision by shifting the decimal place in
  // the original number before the round, then moving it back after the round.
  int precision = 0;
  double original_value = AS_NUMBER(args[0]);
  if (arg_count == 2 && IS_NUMBER(args[1])) {
    // This rounds the precision towards zero and then slams it into a 16bit
    // int. Not gonna care in the slightest about possible precision lost.
    precision = (int)trunc(AS_NUMBER(args[1]));
    // The actual shifting is done by playing with the power of 10.  If we are
    // given a precision of 5, we'll multiply the incoming number by 10^5 =
    // 10000.
    original_value = original_value * pow(10, precision);
  }

  // The actual rounding method we'll use is finding the nearest integer.
  // The C spec allows all of the rounding methods to be implementation defined.
  // It is unclear which internal tiebreaking mechanism we're going to get.
  int old_rounding_mode = fegetround();
  fesetround(FE_TONEAREST);
  double new_value = nearbyint(original_value);
  fesetround(old_rounding_mode);

  if (precision != 0) {
    // If we shifted the decimal place for precision earlier, undo that now.
    new_value = new_value * pow(10, -1 * precision);
  }
  return NUMBER_VAL(new_value);
}


Value cc_function_number_clamp(int arg_count, Value* args) {
  if (arg_count != 3
      || !IS_NUMBER(args[0])
      || !IS_NUMBER(args[1])
      || !IS_NUMBER(args[2])
     ) {
    return NIL_VAL;
  }
  double value = AS_NUMBER(args[0]);
  double minimum = AS_NUMBER(args[1]);
  double maximum = AS_NUMBER(args[2]);
  if (value < minimum) {
    value = minimum;
  }
  if (value > maximum) {
    value = maximum;
  }
  return NUMBER_VAL(value);
}


Value cc_function_number_to_string(int arg_count, Value* args) {
  if (arg_count != 1 || !IS_NUMBER(args[0])) {
    return NIL_VAL;
  }
  double number = AS_NUMBER(args[0]);
  // Oh how I loathe manual memory management!
  // Taking this tip from: https://stackoverflow.com/a/3923207/16886
  // 16 is where things get wonky for precision, so let's cut it off before then.
  int buffer_size = snprintf(NULL, 0, "%.15g", number);
  char *buffer = ALLOCATE(char, 1 + buffer_size);
  sprintf(buffer, "%.15g", number);
  buffer[buffer_size] = '\0';
  // takeString converts our memory-managed string value into an interned
  // ObjString, freeing it up if it was a dupe.
  return OBJ_VAL(takeString(buffer, buffer_size));
}


Value cc_function_number_to_hex_string(int arg_count, Value* args) {
  if (arg_count != 1 || !IS_NUMBER(args[0])) {
    return NIL_VAL;
  }
  double raw_number = AS_NUMBER(args[0]);
  // The method we're going to use to hexify this number can't take negatives.
  if (raw_number < 0 || isnan(raw_number) || isinf(raw_number)) {
    return NIL_VAL;
  }
  // The hex formatter for doubles wants to prefix "0x" or "0X" into the string,
  // which is dumb.  To use the "%x" formatter, which just emits hex digits,
  // we're going to have to pass it an integer.  As doubles can represent
  // integers that take more than 32 bits to represent, we'll have to use a
  // 64-bit integer to store it.
  uint64_t number = (uint64_t)trunc(raw_number);

  // Same buffer size trick as number_to_string
  int buffer_size = snprintf(NULL, 0, "%lx", number);
  char *buffer = ALLOCATE(char, 1 + buffer_size);
  sprintf(buffer, "%lx", number);
  return OBJ_VAL(takeString(buffer, buffer_size));
}

#ifdef FEATURE_USER_HASHES
Value cc_function_ht_create(int arg_count, Value* args) {
  ObjUserHash* desu = newUserHash();
  Value spam = OBJ_VAL(desu);
  return spam;
}


Value cc_function_ht_set(int arg_count, Value* args) {
  if(arg_count != 3 || !IS_USERHASH(args[0]) || !IS_STRING(args[1])) {
    return NIL_VAL;
  }
  ObjUserHash* hash = AS_USERHASH(args[0]);
  // tableSet returns true if it's a new key, but we don't care here.
  tableSet(&hash->table, AS_STRING(args[1]), args[2]);
  return BOOL_VAL(true);
}


Value cc_function_ht_has(int arg_count, Value* args) {
  if(arg_count != 2 || !IS_USERHASH(args[0]) || !IS_STRING(args[1])) {
    return NIL_VAL;
  }
  ObjUserHash* hash = AS_USERHASH(args[0]);
  // tableGet returns true if the key exists, which is what we want.
  Value unused = NIL_VAL;
  return BOOL_VAL( tableGet(&hash->table, AS_STRING(args[1]), &unused) );
}


Value cc_function_ht_update(int arg_count, Value* args) {
  if(arg_count != 3 || !IS_USERHASH(args[0]) || !IS_STRING(args[1])) {
    return NIL_VAL;
  }
  ObjUserHash* hash = AS_USERHASH(args[0]);
  Value old_value = NIL_VAL;
  // Hmm, maybe there's a case for a set-and-return-if-defined?
  tableGet(&hash->table, AS_STRING(args[1]), &old_value);
  tableSet(&hash->table, AS_STRING(args[1]), args[2]);
  return old_value;
}


Value cc_function_ht_unset(int arg_count, Value* args) {
  if(arg_count != 2 || !IS_USERHASH(args[0]) || !IS_STRING(args[1])) {
    return NIL_VAL;
  }
  ObjUserHash* hash = AS_USERHASH(args[0]);
  return BOOL_VAL( tableDelete(&hash->table, AS_STRING(args[1])) );
}


Value cc_function_ht_get(int arg_count, Value* args) {
  if(arg_count != 2 || !IS_USERHASH(args[0]) || !IS_STRING(args[1])) {
    return NIL_VAL;
  }
  ObjUserHash* hash = AS_USERHASH(args[0]);
  Value result = NIL_VAL;
  tableGet(&hash->table, AS_STRING(args[1]), &result);
  return result;
}


Value cc_function_ht_count_keys(int arg_count, Value* args) {
  if(arg_count != 1 || !IS_USERHASH(args[0])) {
    return NIL_VAL;
  }
  ObjUserHash* hash = AS_USERHASH(args[0]);
  return NUMBER_VAL(hash->table.count - hash->table.tombstone_count);
}


Value cc_function_ht_clear(int arg_count, Value* args) {
  if(arg_count != 1 || !IS_USERHASH(args[0])) {
    return NIL_VAL;
  }
  ObjUserHash* hash = AS_USERHASH(args[0]);
  freeTable(&hash->table);
  initTable(&hash->table);
  return BOOL_VAL(true);
}


Value cc_function_ht_get_key_index(int arg_count, Value* args) {
  if(arg_count != 2 || !IS_USERHASH(args[0]) || !IS_NUMBER(args[1])) {
    return NIL_VAL;
  }
  ObjUserHash* hash = AS_USERHASH(args[0]);
  Entry* entries = hash->table.entries;
  // We're dealing with two different indexing methods.
  // The keys for the hash are stored in an array of size .capacity.  However,
  // only .count entries are in that array.  We need to go through the array
  // and skip empty entries, counting only ones that are valid.
  int desired_index = (int)AS_NUMBER(args[1]);
  int entry_count = 0;
  bool found = false;
  int found_index = 0;
  for(int i = 0; i < hash->table.capacity; i++) {
    // Skip emptry entries and tombstones
    if(entries[i].key == NULL) {
      continue;
    }
    // Otherwise we found a valid entry.  If it's the one we're looking for, stop.
    if(entry_count == desired_index) {
      found = true;
      found_index = i;
      break;
    }
    // We found an entry, but it's not the one we're looking for.
    entry_count++;
  }
  if(!found) {
    return BOOL_VAL(false);
  }
  return OBJ_VAL(entries[found_index].key);
}
#endif

static bool random_seeded = false;
Value cc_function_number_random(int arg_count, Value* args) {
  if(arg_count != 2 || !IS_NUMBER(args[0]) || !IS_NUMBER(args[1])) {
    return NIL_VAL;
  }
  if(!random_seeded) {
    srand48(time(0));
    random_seeded = true;
  }
  // We're turning a floating point value between 0 and 1 into an integer
  // between the two passed values, which we will assume are integers themselves.
  // Because we're truncating the final value, we need to bump the greater
  // number up by one to make sure that it's included in the result.  The
  // distribution of this looks pretty even based on some very basic testing.
  double greater = 1 + fmax( AS_NUMBER(args[0]), AS_NUMBER(args[1]) );
  double lesser = fmin( AS_NUMBER(args[0]), AS_NUMBER(args[1]) );
  double range = greater - lesser;
  double new_value = trunc( (drand48() * range) + lesser );

  return NUMBER_VAL( new_value );
}


void cc_register_functions() {
  defineNative("string_substring",      cc_function_string_substring);
  defineNative("string_length",         cc_function_string_length);
  defineNative("debug_dump_stack",      cc_function_debug_dump_stack);
  defineNative("time",                  cc_function_time);
  defineNative("environment_getvar",    cc_function_environment_getvar);

  defineNative("val_is_empty",          cc_function_val_is_empty);
  defineNative("val_is_string",         cc_function_val_is_string);
#ifdef FEATURE_USER_HASHES
  defineNative("val_is_userhash",       cc_function_val_is_userhash);
#endif
  defineNative("val_is_number",         cc_function_val_is_number);
  defineNative("val_is_boolean",        cc_function_val_is_boolean);
  defineNative("val_is_nan",            cc_function_val_is_nan);
  defineNative("val_is_infinity",       cc_function_val_is_infinity);

  defineNative("number_absolute",       cc_function_number_absolute);
  defineNative("number_remainder",      cc_function_number_remainder);
  defineNative("number_minimum",        cc_function_number_minimum);
  defineNative("number_maximum",        cc_function_number_maximum);
  defineNative("number_floor",          cc_function_number_floor);
  defineNative("number_ceiling",        cc_function_number_ceiling);
  defineNative("number_round",          cc_function_number_round);
  defineNative("number_clamp",          cc_function_number_clamp);
  defineNative("number_to_string",      cc_function_number_to_string);
  defineNative("number_to_hex_string",  cc_function_number_to_hex_string);
  defineNative("number_random",         cc_function_number_random);

#ifdef FEATURE_USER_HASHES
  defineNative("ht_create",             cc_function_ht_create);
  defineNative("ht_set",                cc_function_ht_set);
  defineNative("ht_has",                cc_function_ht_has);
  defineNative("ht_update",             cc_function_ht_update);
  defineNative("ht_unset",              cc_function_ht_unset);
  defineNative("ht_get",                cc_function_ht_get);
  defineNative("ht_count_keys",         cc_function_ht_count_keys);
  defineNative("ht_clear",              cc_function_ht_clear);
  defineNative("ht_get_key_index",      cc_function_ht_get_key_index);
#endif
}
