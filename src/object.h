#ifndef clox_object_h
#define clox_object_h

#include <stdio.h>
#include <fcntl.h>

#include "common.h"
#include "chunk.h"
#include "value.h"
#include "table.h"

#define OBJ_TYPE(value)         (AS_OBJ(value)->type)

#define IS_FUNCTION(value)      isObjType(value, OBJ_FUNCTION)
#define IS_NATIVE(value)        isObjType(value, OBJ_NATIVE)
#define IS_STRING(value)        isObjType(value, OBJ_STRING)

#ifdef CC_FEATURES
#define IS_USERHASH(value)      isObjType(value, OBJ_USERHASH)
#define IS_USERARRAY(value)     isObjType(value, OBJ_USERARRAY)
#define IS_FILEHANDLE(value)    isObjType(value, OBJ_FILEHANDLE)
#endif

#define AS_FUNCTION(value)      ((ObjFunction*)AS_OBJ(value))
#define AS_NATIVE(value)        (((ObjNative*)AS_OBJ(value))->function)
#define AS_STRING(value)        ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value)       (((ObjString*)AS_OBJ(value))->chars)

#ifdef CC_FEATURES
#define AS_USERHASH(value)      ((ObjUserHash*)AS_OBJ(value))
#define AS_USERARRAY(value)     ((ObjUserArray*)AS_OBJ(value))
#define AS_FILEHANDLE(value)    ((ObjFileHandle*)AS_OBJ(value))
#endif

typedef enum {
  OBJ_FUNCTION,
  OBJ_NATIVE,
  OBJ_STRING,
#ifdef CC_FEATURES
  OBJ_USERHASH,
  OBJ_USERARRAY,
  OBJ_FILEHANDLE,
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

#ifdef CC_FEATURES
typedef struct {
  Obj obj;
  Table table;
} ObjUserHash;

typedef struct {
  Obj obj;
  ValueArray inner;
} ObjUserArray;

typedef struct {
  Obj obj;
  FILE* handle;
  struct flock* lock;
} ObjFileHandle;
#endif

struct sObjString {
  Obj obj;
  int length;
  char* chars;
  uint32_t hash;
};


#ifdef CC_FEATURES
ObjUserHash* newUserHash();
ObjUserArray* newUserArray();
ObjFileHandle* newFileHandle(FILE* handle, struct flock* lock);
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
