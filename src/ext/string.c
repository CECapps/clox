#include <stdlib.h>

#include "../common.h"
#include "../memory.h"
#include "../vm.h"

static int16_t st_normalize_index(ObjString* str, double raw_index) {
  // ObjString's length is held in an int, which is apparently 16 bits?
  if(raw_index > INT16_MAX || raw_index < INT16_MIN) {
    return -1;
  }
  int16_t target_index = (int16_t)raw_index;
  // Given a string length of 8 and a starting index of -5,
  // (-5) + 8 = 3
  // [0]  [1]  [2]  [3]  [4]  [5]  [6]  [7]
  // -8   -7   -6   -5   -4   -3   -2   -1
  if(target_index < 0) {
    target_index += str->length;
  }
  // Most of the time we should be inside the string.
  if(target_index >= 0 && target_index < str->length) {
    return target_index;
  }
  // The target index will always be positive, so returning -1 will always raise
  // an error, not refer to index -1.
  return -1;
}


static int16_t st_normalize_range(ObjString* str, int16_t index, double raw_range) {
  if(raw_range > INT16_MAX || raw_range < INT16_MIN) {
    // @FIXME This is an error, but there's no way of reporting it.  Returning a
    // range of zero is going to have to do for now.
    return 0;
  }
  int16_t range = (int16_t)raw_range;
  // Given a string length of 8 and a starting index of 3, the maximum positive
  // range is 5, and the maximum negative range is -4.
  // Yes, range includes the index.
  int16_t span = index + range;
  if(span > str->length) {
    // The range is too high.  Clamp it to the end of the string.
    range = str->length - index;
  } else if(span < 0) {
    // The negative range would wrap if it could, but it can't.  Clamp it to the
    // start of the string instead.
    range = index * -1;
  }
  return range;
}


struct ST_Legal_Range {
    int16_t index;
    int16_t range;
    bool error;
};

static struct ST_Legal_Range st_normalize_index_range(
  ObjString* str, double raw_index, double raw_range
) {
  struct ST_Legal_Range res;
  res.error = false;

  res.index = st_normalize_index(str, raw_index);
  if(res.index == -1) {
    res.error = true;
    res.index = 0;
  }
  // We could get back a negative range, but we always want the range to be
  // positive.  Invert the range and then move the index left.  This will cover
  // the same span.
  res.range = st_normalize_range(str, res.index, raw_range);
  if(res.range < 0 && !res.error) {
    res.range *= -1;
    res.index -= res.range;
  }
  return res;
}


/**
 * string_substring(string, index, count?)
 * - returns nil on parameter error
 * - returns false if the given index out of legal range
 * - returns a copy of the given string starting at the given index for up to
 *   count characters.  If count is omitted, then the entire remaining string
 *   is returned instead.  Both the count and index may be negative, but both
 *   must be inside the legal range.
 */
Value cc_function_string_substring(int arg_count, Value* args) {
  if(arg_count < 2 || arg_count > 3 || !IS_STRING(args[0]) || !IS_NUMBER(args[1]))  {
    return NIL_VAL;
  }

  ObjString* str = AS_STRING(args[0]);

  double raw_range = 0;
  if(arg_count == 3 && IS_NUMBER(args[2])) {
    raw_range = AS_NUMBER(args[2]);
  }
  struct ST_Legal_Range legal = st_normalize_index_range(str, AS_NUMBER(args[1]), raw_range);
  if(legal.error) {
    return BOOL_VAL(false);
  }
  // Zero range is both our default and the error state, but that's fine.
  // When it's zero, we're just going to take the remainder of the string.
  int16_t count = legal.range;
  if(count == 0) {
    count = str->length - legal.index;
  }

  char* new_chars = ALLOCATE(char, count);
  for(int i = 0; i < count; i++) {
    new_chars[i] = str->chars[i + legal.index];
  }
  new_chars[count] = '\0';
  return OBJ_VAL(takeString(new_chars, count));
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

