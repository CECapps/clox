#include <stdlib.h>

#include "common.h"
#include "memory.h"
#include "vm.h"

void* reallocate(void* previous, size_t oldSize, size_t newSize) {
  if (newSize == 0) {
    free(previous);
    return NULL;
  }

  return realloc(previous, newSize);
}


static void freeObject(Obj* object) {
  switch (object->type) {

    case OBJ_FUNCTION: {
      ObjFunction* function = (ObjFunction*)object;
      freeChunk(&function->chunk);
      FREE(ObjFunction, object);
      break;
    }

    case OBJ_NATIVE:
      FREE(ObjNative, object);
      break;

    case OBJ_STRING: {
      ObjString* string = (ObjString*)object;
      FREE_ARRAY(char, string->chars, string->length + 1);
      FREE(ObjString, object);
      break;
    }

    case OBJ_USERARRAY: {
      ObjUserArray* ua = (ObjUserArray*)object;
      freeValueArray(&ua->inner);
      FREE(ObjUserArray, object);
      break;
    }

    case OBJ_USERHASH: {
      ObjUserHash* ht = (ObjUserHash*)object;
      freeTable(&ht->table);
      FREE(ObjUserHash, object);
      break;
    }

    case OBJ_FILEHANDLE: {
      FREE(ObjFileHandle, object);
      break;
    }

    case OBJ_FERROR: {
      FREE(ObjFunctionError, object);
      break;
    }

  }
}


void freeObjects() {
  Obj* object = vm.objects;
  while (object != NULL) {
    Obj* next = object->next;
    freeObject(object);
    object = next;
  }
}
