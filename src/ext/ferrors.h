#ifndef cc_ext_ferrors_h
#define cc_ext_ferrors_h

#include <errno.h>

typedef enum {
    FE_NONE,

    FE_INVALID_ARGUMENTS,
    FE_ARG_GTE_1,
    FE_ARG_COUNT_1,
    FE_ARG_COUNT_2,
    FE_ARG_COUNT_3,
    FE_ARG_COUNT_1_2,
    FE_ARG_COUNT_2_3,
    FE_ARG_1_STRING,
    FE_ARG_1_FH,
    FE_ARG_1_NUMBER,
    FE_ARG_1_ARRAY,
    FE_ARG_2_STRING,
    FE_ARG_2_NUMBER,
    FE_ARG_2_ARRAY,
    FE_ARG_2_FUNCTION,
    FE_ARG_3_NUMBER,
    FE_ARG_3_ARRAY,

    FE_STRING_SPLIT_NEGATIVE_LIMIT,

    FE_FOPEN_MUST_READ_OR_WRITE,
    FE_FOPEN_OPEN_FAILED,
    FE_FOPEN_FDOPEN_FAILED,
    FE_FOPEN_FLOCK_FAILED,
    FE_FCLOSE_FLUSH_FAILED,
    FE_FCLOSE_UNFLOCK_FAILED,
    FE_FCLOSE_FCLOSE_FAILED_LOL,
    FE_FREAD_GETLINE_FAILED,

    FE_INVALID_ERROR_ID,
    FE_MAX_ERROR_ID
} FErrorIDs;

#define FERROR_VAL(ferror_id) OBJ_VAL(newFunctionError(ferror_id, 0))
#define FERROR_ERRNO_VAL(ferror_id) OBJ_VAL(newFunctionError(ferror_id, errno))

const char* cc_ferror_to_string(int ferror_id);

#endif