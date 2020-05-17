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
      for(int i = 0; i < ua->inner.capacity; i++) {
        // Every element in the array is guaranteed to be a Value.
        if(IS_OBJ(ua->inner.values[i])) {
          freeObject(AS_OBJ(ua->inner.values[i]));
        }
        FREE(Value, &ua->inner.values[i]);
      }
      FREE_ARRAY(Value, ua->inner.values, ua->inner.capacity);
      initValueArray(&ua->inner);
      FREE(ObjUserArray, object);
    }

    case OBJ_USERHASH: {
      ObjUserHash* ht = (ObjUserHash*)object;
      // The table is an array of Entries.  Each Entry has a key and a value.
      // The key may be either NULL or an ObjString.  The value is *always* a
      // valid Value.
      for(int i = 0; i < ht->table.capacity; i++) {
        if(ht->table.entries[i].key != NULL) {
          // It's a string key, so treat it like a string.
          freeObject((Obj*) ht->table.entries[i].key);
        }
        if(IS_OBJ(ht->table.entries[i].value)) {
          freeObject(AS_OBJ(ht->table.entries[i].value));
        }
        FREE(Value, &ht->table.entries[i].value);
      }
      FREE_ARRAY(Entry, ht->table.entries, ht->table.capacity);
      initTable(&ht->table);
      FREE(ObjUserHash, object);
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
