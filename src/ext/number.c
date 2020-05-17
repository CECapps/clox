#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <fenv.h>

#include "../memory.h"
#include "../vm.h"
#include "./ferrors.h"


Value cc_function_number_absolute(int arg_count, Value* args) {
  if (arg_count != 1 || !IS_NUMBER(args[0])) {
    return FERROR_VAL(FE_ARG_1_NUMBER);
  }
  return NUMBER_VAL(fabs(AS_NUMBER(args[0])));
}


Value cc_function_number_remainder(int arg_count, Value* args) {
  if (arg_count != 2) { return FERROR_VAL(FE_ARG_COUNT_2); }
  if (!IS_NUMBER(args[0])) { return FERROR_VAL(FE_ARG_1_NUMBER); }
  if (!IS_NUMBER(args[1])) { return FERROR_VAL(FE_ARG_2_NUMBER); }

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
    return FERROR_VAL(FE_ARG_GTE_1);
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
    return FERROR_VAL(FE_ARG_GTE_1);
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
    return FERROR_VAL(FE_ARG_1_NUMBER);
  }
  return NUMBER_VAL(floor(AS_NUMBER(args[0])));
}


Value cc_function_number_ceiling(int arg_count, Value* args) {
  if (arg_count != 1 || !IS_NUMBER(args[0])) {
    return FERROR_VAL(FE_ARG_1_NUMBER);
  }
  return NUMBER_VAL(ceil(AS_NUMBER(args[0])));
}


Value cc_function_number_round(int arg_count, Value* args) {
  if (arg_count < 1 || !IS_NUMBER(args[0])) { return FERROR_VAL(FE_ARG_1_NUMBER); }
  if (arg_count == 2 && !IS_NUMBER(args[1])) { return FERROR_VAL(FE_ARG_2_NUMBER); }
  if (arg_count > 2) { return FERROR_VAL(FE_ARG_COUNT_1_2); }

  // We'll optionally be passed a precision argument, which may be a positive
  // or negative integer.  Our stdlib functions only operate on integers.
  // We can simulate the requested precision by shifting the decimal place in
  // the original number before the round, then moving it back after the round.
  int precision = 0;
  double original_value = AS_NUMBER(args[0]);
  if (arg_count == 2) {
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
  if (arg_count != 3) { return FERROR_VAL(FE_ARG_COUNT_3); }
  if (!IS_NUMBER(args[0])) { return FERROR_VAL(FE_ARG_1_NUMBER); }
  if (!IS_NUMBER(args[1])) { return FERROR_VAL(FE_ARG_2_NUMBER); }
  if (!IS_NUMBER(args[2])) { return FERROR_VAL(FE_ARG_3_NUMBER); }

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
    return FERROR_VAL(FE_ARG_1_NUMBER);
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
  // ObjString, freeing it up if it was a dupe and using it directly otherwise.
  // (That is, there is no need to free the buffer here.)
  ObjString* str = takeString(buffer, buffer_size);
  return OBJ_VAL(str);
}


Value cc_function_number_to_hex_string(int arg_count, Value* args) {
  if (arg_count != 1 || !IS_NUMBER(args[0])) {
    return FERROR_VAL(FE_ARG_1_NUMBER);
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

  // Same buffer size + allocation trick as number_to_string
  int buffer_size = snprintf(NULL, 0, "%lx", number);
  char *buffer = ALLOCATE(char, 1 + buffer_size);
  sprintf(buffer, "%lx", number);
  buffer[buffer_size] = '\0';
  return OBJ_VAL(takeString(buffer, buffer_size));
}


static bool random_seeded = false;
double rng() {
  if(!random_seeded) {
    srand48(time(0));
    random_seeded = true;
  }
  return drand48();
}


double random_int(double left, double right) {
  // We're turning a floating point value between 0 and 1 into an integer
  // between the two passed values, which we will assume are integers themselves.
  // Because we're truncating the final value, we need to bump the greater
  // number up by one to make sure that it's included in the result.  The
  // distribution of this looks pretty even based on some very basic testing.
  double greater = 1 + fmax(left, right);
  double lesser = fmin(left, right);
  double range = greater - lesser;
  return trunc( (rng() * range) + lesser );
}


Value cc_function_number_random(int arg_count, Value* args) {
  if (arg_count != 2) { return FERROR_VAL(FE_ARG_COUNT_2); }
  if (!IS_NUMBER(args[0])) { return FERROR_VAL(FE_ARG_1_NUMBER); }
  if (!IS_NUMBER(args[1])) { return FERROR_VAL(FE_ARG_2_NUMBER); }

  return NUMBER_VAL( random_int( AS_NUMBER(args[0]), AS_NUMBER(args[1]) ) );
}


void cc_register_ext_number() {
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
}
