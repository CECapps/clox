#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <sys/time.h>
#include <math.h>
#include <fenv.h>

#include "../common.h"
#include "../compiler.h"
#include "../debug.h"
#include "../object.h"
#include "../memory.h"
#include "../table.h"
#include "../vm.h"


Value cc_function_val_is_userhash(int arg_count, Value* args) {
    if(arg_count != 1) {
        return NIL_VAL;
    }
    return BOOL_VAL( IS_USERHASH(args[0]) );
}


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


void cc_register_ext_userhash() {
  defineNative("val_is_userhash",  cc_function_val_is_userhash);
  defineNative("ht_create",        cc_function_ht_create);
  defineNative("ht_set",           cc_function_ht_set);
  defineNative("ht_has",           cc_function_ht_has);
  defineNative("ht_update",        cc_function_ht_update);
  defineNative("ht_unset",         cc_function_ht_unset);
  defineNative("ht_get",           cc_function_ht_get);
  defineNative("ht_count_keys",    cc_function_ht_count_keys);
  defineNative("ht_clear",         cc_function_ht_clear);
  defineNative("ht_get_key_index", cc_function_ht_get_key_index);
}
