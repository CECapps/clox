/*
    "As all functions from Dynamic Memory TR, getline is only guaranteed to be
    available if __STDC_ALLOC_LIB__ is defined by the implementation and if the
    user defines __STDC_WANT_LIB_EXT2__ to the integer constant 1 before
    including stdio.h."
*/
#define __STDC_WANT_LIB_EXT2__ 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include "../common.h"
#include "../vm.h"

#include "ferrors.h"

/**
 * file_open(filename, mode)
 * - returns nil on parameter error
 * - returns false on file open error
 * - returns filehandle on success
 * - mode may be:
 *   - "r" to read from the file, or fail if it does not exist
 *   - "w" to write to the file, creating it if it does not exist
 *     - **NOTE** "w" does *not* truncate the file!
 *   - "rw" to read and write from the file, creating it if it does not exist
 * - "r" mode establishes a shared lock on the file.
 * - the "w" modes establish an exclusive lock on the file
 * - files are opened in normal mode, performing newline interpolation
 *   - ideally they'd actually be opened in binary mode, but that makes reading
 *     lines delimited by things other than newlines (like CRLF) really annoying
 */
Value cc_function_file_open(int arg_count, Value* args) {
    if(arg_count < 1 || arg_count > 2) { return FERROR_VAL(FE_ARG_COUNT_1_2); }
    if(!IS_STRING(args[0])) { return FERROR_VAL(FE_ARG_1_STRING); }
    if(arg_count == 2 && !IS_STRING(args[1])) { return FERROR_VAL(FE_ARG_2_STRING); }

    // We only support passing read, write, or both.  All other flags are ignored.
    bool is_reader = true;
    bool is_writer = false;
    if(arg_count == 2) {
        is_reader = !(strstr(AS_CSTRING(args[1]), "r") == NULL);
        is_writer = !(strstr(AS_CSTRING(args[1]), "w") == NULL);
    }
    // Can't do nothing.
    if(!is_reader && !is_writer) {
        return FERROR_VAL(FE_FOPEN_MUST_READ_OR_WRITE);
    }
    // In order to get the "open for reading and writing, creating but not
    // truncating" behavior we desire, we need to get a bit lower level.
    int open_mode = is_reader && !is_writer
                    ? O_RDONLY
                    : (is_writer && !is_reader ? O_WRONLY : O_RDWR);
    if(is_writer) {
        open_mode = open_mode | O_CREAT;
    }
    int fd = open(AS_CSTRING(args[0]), open_mode);
    if(fd == -1) {
        return FERROR_ERRNO_VAL(FE_FOPEN_OPEN_FAILED);
    }
    // Passing "w" to fopen causes a truncate, but passing it to fdopen does not.
    char* fopen_mode = is_reader && !is_writer
                    ? "r"
                    : (is_writer && !is_reader ? "w" : "r+");
    FILE* handle = fdopen(fd, fopen_mode);
    if(handle == NULL) {
        return FERROR_ERRNO_VAL(FE_FOPEN_FDOPEN_FAILED);
    }

    // POSIX file locks seem to be a better (more portable) choice than flock()
    struct flock file_lock = {
        .l_type   = is_writer ? F_WRLCK : F_RDLCK,
        .l_whence = SEEK_SET, // POSIX file locking is range based, so...
        .l_start  = 0,        // ...start our lock at the start of the file...
        .l_len    = 0,        // ...and extend it through the end.  0 is magic.
        .l_pid    = getpid()
    };
    int flockres = fcntl(fileno(handle), F_SETLKW, &file_lock); // Set Lock, Wait (blocking)
    if(flockres != 0) {
        return FERROR_ERRNO_VAL(FE_FOPEN_FLOCK_FAILED);
    }
    // Okay, we should be good to go now.
    return OBJ_VAL(newFileHandle(handle, &file_lock));
}


/**
 * file_close(fh)
 * - returns nil on parameter error
 * - returns true on success... yes, this can fail.  Don't ask how.
 */
Value cc_function_file_close(int arg_count, Value* args) {
    if(arg_count != 1 || !IS_FILEHANDLE(args[0])) {
        return FERROR_VAL(FE_ARG_1_FH);
    }

    ObjFileHandle* fh = AS_FILEHANDLE(args[0]);
    // The close automatically flushes, but let's do that *before* the unlock.
    if(fflush(fh->handle) != 0) {
        return FERROR_ERRNO_VAL(FE_FCLOSE_FLUSH_FAILED);
    }
    // Because POSIX locking is region based, it's best to just change the lock
    // type in the original lock data and pass it back through.
    fh->lock->l_type = F_UNLCK;
    if(fcntl(fileno(fh->handle), F_SETLK, fh->lock) != 0) {
        return FERROR_ERRNO_VAL(FE_FCLOSE_UNFLOCK_FAILED);
    }
    // Now we can finally actually close the handle.  Yes, it's safe to use
    // fclose() here instead of close().
    if(fclose(fh->handle) != 0) {
        return FERROR_ERRNO_VAL(FE_FCLOSE_FCLOSE_FAILED_LOL);
    }
    return BOOL_VAL(true);
}


/**
 * file_read_line(fh)
 * - returns nil on parameter error
 * - returns false on end of file
 * - returns the string next line otherwise, including the newline
 */
Value cc_function_file_read_line(int arg_count, Value* args) {
    if(arg_count != 1 || !IS_FILEHANDLE(args[0])) {
        return FERROR_VAL(FE_ARG_1_FH);
    }
    ObjFileHandle* fh = AS_FILEHANDLE(args[0]);

    // Don't even bother if we've reached EOF
    if(feof(fh->handle) != 0) {
        return BOOL_VAL(false);
    }

    char* buffer = NULL;
    size_t bufflen = 0;
    ssize_t res = getline(&buffer, &bufflen, fh->handle);
    if(res < 0) {
        // So we should have been given the number of bytes put into the buffer,
        // but instead we got an error.  This can happen if we are at EOF and
        // we tried unsuccessfully to read, or if there was some sort of error.
        // The EOF check at the top should have protected us from fuckery...
        free(buffer);
        return FERROR_VAL(FE_FREAD_GETLINE_FAILED);
    }
    ObjString* str = copyString(buffer, bufflen);
    free(buffer);
    return OBJ_VAL(str);
}


/**
 * file_at_eof(fh)
 * - returns nil on parameter error
 * - returns true if the end of the file has been reached
 */
Value cc_function_file_at_eof(int arg_count, Value* args) {
    if(arg_count != 1 || !IS_FILEHANDLE(args[0])) {
        return FERROR_VAL(FE_ARG_1_FH);
    }

    return BOOL_VAL(feof(AS_FILEHANDLE(args[0])->handle) != 0);
}


/**
 * file_write(fh, data)
 * - returns nil on error
 * - returns the number of bytes written
 */
Value cc_function_file_write(int arg_count, Value* args) { return NIL_VAL; }


/**
 * file_truncate(fh, length = 0)
 * - returns nil on error
 * - returns true on success
 */
Value cc_function_file_truncate(int arg_count, Value* args) { return NIL_VAL; }


/**
 * file_exists(filename)
 * - returns nil on error
 * - returns true if the given filename exists
 */
Value cc_function_file_exists(int arg_count, Value* args) { return NIL_VAL; }


/**
 * file_read_block(fh, max_length?)
 * - returns nil on error
 * - returns false on end of file
 * - returns a string containing up to max_length bytes
 *   - if max_length is missing or zero, the entire file will be read
 */
Value cc_function_file_read_block(int arg_count, Value* args) { return NIL_VAL; }


/**
 * file_position(fh)
 * - returns nil on error
 * - returns the position of the current filehandle within the file, in bytes
 */
Value cc_function_file_position(int arg_count, Value* args) { return NIL_VAL; }


/**
 * file_seek(fh, relative_position)
 * - returns nil on error
 * - returns true on success
 */
Value cc_function_file_seek(int arg_count, Value* args) { return NIL_VAL; }


/**
 * file_seek_start(fh, offset?)
 * - returns nil on error
 * - returns true on success
 * - moves the file pointer to the start of the file, plus the given offset
 */
Value cc_function_file_seek_start(int arg_count, Value* args) { return NIL_VAL; }


/**
 * file_seek_end(fh, offset?)
 * - returns nil on error
 * - returns true on success
 * - moves the file pointer to the end of the file, plus the given offset
 */
Value cc_function_file_seek_end(int arg_count, Value* args) { return NIL_VAL; }


void cc_register_ext_file() {
    defineNative("file_open",           cc_function_file_open);
    defineNative("file_close",          cc_function_file_close);
    defineNative("file_read_line",      cc_function_file_read_line);
    defineNative("file_at_eof",         cc_function_file_at_eof);
    defineNative("file_write",          cc_function_file_write);
    defineNative("file_truncate",       cc_function_file_truncate);
    defineNative("file_exists",         cc_function_file_exists);
    defineNative("file_read_block",     cc_function_file_read_block);
    defineNative("file_position",       cc_function_file_position);
    defineNative("file_seek",           cc_function_file_seek);
    defineNative("file_seek_start",     cc_function_file_seek_start);
    defineNative("file_seek_end",       cc_function_file_seek_end);
}
