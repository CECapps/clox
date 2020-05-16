#ifndef cc_ext_ferrors_h
#define cc_ext_ferrors_h

#include <errno.h>

typedef enum {
    FE_NONE,

    FE_INVALID_ARGUMENTS,

    FE_FOPEN_MUST_READ_OR_WRITE,
    FE_FOPEN_OPEN_FAILED,
    FE_FOPEN_FDOPEN_FAILED,
    FE_FOPEN_FLOCK_FAILED,

    FE_INVALID_ERROR_ID,
    FE_MAX_ERROR_ID
} FErrorIDs;

#define FERROR_VAL(ferror_id) OBJ_VAL(newFunctionError(ferror_id, 0))
#define FERROR_ERRNO_VAL(ferror_id) OBJ_VAL(newFunctionError(ferror_id, errno))

const char* cc_ferror_to_string(int ferror_id);

#endif
