#include "ferrors.h"

const char* ferror_strings[FE_MAX_ERROR_ID] = {
    /* FE_NONE */                       "(no error)",

    /* FE_INVALID_ARGUMENTS */          "invalid function arguments",

    /* FE_FOPEN_MUST_READ_OR_WRITE */   "must include read mode or write mode flags",
    /* FE_FOPEN_OPEN_FAILED */          "call to open() failed",
    /* FE_FOPEN_FDOPEN_FAILED */        "call to fdopen() failed",
    /* FE_FOPEN_FLOCK_FAILED */         "obtaining POSIX file lock failed",

    /* FE_INVALID_ERROR_ID */           "(invalid ferror_id)",
};

const char* cc_ferror_to_string(int ferror_id) {
    if(ferror_id >= FE_MAX_ERROR_ID) {
        ferror_id = FE_INVALID_ERROR_ID;
    }
    return ferror_strings[ferror_id];
}
