#include <stdlib.h>

#include "../common.h"
#include "../memory.h"
#include "../vm.h"


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


void cc_register_ext_string() {
  defineNative("string_substring", cc_function_string_substring);
  defineNative("string_length",    cc_function_string_length);
}
