#include "ferrors.h"

const char* ferror_strings[FE_MAX_ERROR_ID] = {
/*  FE_NONE */                       "(no error)"

/*  FE_INVALID_ARGUMENTS */          ,"invalid function arguments"
/*  FE_ARG_GTE_1 */                  ,"function takes at least one argument"
/*  FE_ARG_COUNT_1 */                ,"function takes 1 argument"
/*  FE_ARG_COUNT_2 */                ,"function takes 2 arguments"
/*  FE_ARG_COUNT_3 */                ,"function takes 3 arguments"
/*  FE_ARG_COUNT_1_2 */              ,"function takes 1 to 2 arguments"
/*  FE_ARG_COUNT_2_3 */              ,"function takes 2 to 3 arguments"
/*  FE_ARG_1_STRING */               ,"first argument must be a string"
/*  FE_ARG_1_FH */                   ,"first argument must be a filehandle"
/*  FE_ARG_1_NUMBER */               ,"first argument must be a number"
/*  FE_ARG_1_ARRAY */                ,"first argument must be an array"
/*  FE_ARG_2_STRING */               ,"second argument must be a string"
/*  FE_ARG_2_NUMBER */               ,"second argument must be a number"
/*  FE_ARG_2_ARRAY */                ,"second argument must be an array"
/*  FE_ARG_2_FUNCTION */             ,"second argument must be a function"
/*  FE_ARG_3_NUMBER */               ,"third argument must be a number"
/*  FE_ARG_3_ARRAY */                ,"third argument must be an array"

/*  FE_STRING_SPLIT_NEGATIVE_LIMIT */,"limit must be positive, negative number provided"

/*  FE_FOPEN_MUST_READ_OR_WRITE */   ,"must include read mode ('r') or write mode ('w') in flag string"
/*  FE_FOPEN_OPEN_FAILED */          ,"internal call to open() failed"
/*  FE_FOPEN_FDOPEN_FAILED */        ,"internal call to fdopen() failed"
/*  FE_FOPEN_FLOCK_FAILED */         ,"obtaining POSIX file lock failed"
/*  FE_FCLOSE_FLUSH_FAILED */        ,"internal call to flush() failed"
/*  FE_FCLOSE_UNFLOCK_FAILED */      ,"clearing POSIX file lock failed"
/*  FE_FCLOSE_FCLOSE_FAILED_LOL */   ,"internal call to fclose() failed (lol how did you do that)"
/*  FE_FREAD_GETLINE_FAILED */       ,"internal call to getline() failed"
/*  FE_FWRITE_FPUTS_FAILED */        ,"internal call to fputs() failed"
/*  FE_DIR_DIROPEN_FAILED, */        ,"internal call to diropen() failed"
/*  FE_DIR_READDIR_FAILED, */        ,"internal call to readdir() failed"
/*  FE_FILE_REALPATH_FAILED */       ,"internal call to realpath() failed"
/*  FE_FILE_STAT_FAILED */           ,"internal call to stat() failed"

/*  FE_PROCESS_PIPE_CREATE_FAILED */ ,"internal call to pipe() failed"
/*  FE_PROCESS_FORK_FAILED */        ,"internal call to fork() failed"
/*  FE_PROCESS_CREATE_FAILED */      ,"process creation failed, internal impossible fallthrough!?"
/*  FE_PROCESS_CLOSE_FAILED */       ,"process close failed"

/*  FE_INVALID_ERROR_ID */           ,"(invalid ferror_id)"
};

const char* cc_ferror_to_string(int ferror_id) {
    if(ferror_id >= FE_MAX_ERROR_ID) {
        ferror_id = FE_INVALID_ERROR_ID;
    }
    return ferror_strings[ferror_id];
}
