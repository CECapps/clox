#ifndef clox_object_h
#define clox_object_h

#include "common.h"
#include "chunk.h"
#include "value.h"
#include "table.h"

#define OBJ_TYPE(value)         (AS_OBJ(value)->type)

#define IS_FUNCTION(value)      isObjType(value, OBJ_FUNCTION)
#define IS_NATIVE(value)        isObjType(value, OBJ_NATIVE)
#define IS_STRING(value)        isObjType(value, OBJ_STRING)

#ifdef FEATURE_USER_HASHES
#define IS_USERHASH(value)      isObjType(value, OBJ_USERHASH)
#endif

#define AS_FUNCTION(value)      ((ObjFunction*)AS_OBJ(value))
#define AS_NATIVE(value)        (((ObjNative*)AS_OBJ(value))->function)
#define AS_STRING(value)        ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value)       (((ObjString*)AS_OBJ(value))->chars)

#ifdef FEATURE_USER_HASHES
#define AS_USERHASH(value)      ((ObjUserHash*)AS_OBJ(value))
#endif

typedef enum {
  OBJ_FUNCTION,
  OBJ_NATIVE,
  OBJ_STRING,
#ifdef FEATURE_USER_HASHES
  OBJ_USERHASH,
#endif
} ObjType;


struct sObj {
  ObjType type;
  struct sObj* next;
};


typedef struct {
  Obj obj;
  int arity;
  Chunk chunk;
  ObjString* name;
} ObjFunction;


typedef Value (*NativeFn)(int argCount, Value* args);


typedef struct {
  Obj obj;
  NativeFn function;
} ObjNative;

#ifdef FEATURE_USER_HASHES
typedef struct {
  Obj obj;
  Table table;
} ObjUserHash;
#endif

struct sObjString {
  Obj obj;
  int length;
  char* chars;
  uint32_t hash;
};


#ifdef FEATURE_USER_HASHES
ObjUserHash* newUserHash();
#endif
ObjFunction* newFunction();
ObjNative* newNative(NativeFn function);
ObjString* takeString(char* chars, int length);
ObjString* copyString(const char* chars, int length);


void printObject(Value value);


static inline bool isObjType(Value value, ObjType type) {
  return IS_OBJ(value) && AS_OBJ(value)->type == type;
}


#endif
