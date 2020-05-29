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
#include <dirent.h>
#include <sys/stat.h>

#include "../common.h"
#include "../memory.h"
#include "../vm.h"

#include "userarray.h"
#include "ferrors.h"


/**
 * file_exists(filename)
 * - returns nil on error
 * - returns true if the given filename exists
 */
Value cc_function_file_exists(int arg_count, Value* args) { return NIL_VAL; }


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
    int fd = open(AS_CSTRING(args[0]), open_mode, 0644);
    if(fd == -1) {
        return FERROR_AUTOERRNO_VAL(FE_FOPEN_OPEN_FAILED);
    }
    // Passing "w" to fopen causes a truncate, but passing it to fdopen does not.
    char* fopen_mode = is_reader && !is_writer
                    ? "r"
                    : (is_writer && !is_reader ? "w" : "r+");
    FILE* handle = fdopen(fd, fopen_mode);
    if(handle == NULL) {
        return FERROR_AUTOERRNO_VAL(FE_FOPEN_FDOPEN_FAILED);
    }
    ObjFileHandle* fh = newFileHandle(handle);
    fh->lock.l_type = is_writer ? F_WRLCK : F_RDLCK;
    fh->is_writer = is_writer;
    fh->is_reader = is_reader;
    fh->is_open = true;

    int flockres = fcntl(fileno(handle), F_SETLKW, &fh->lock); // Set Lock, Wait (blocking)
    if(flockres != 0) {
        return FERROR_AUTOERRNO_VAL(FE_FOPEN_FLOCK_FAILED);
    }
    // Okay, we should be good to go now.
    return OBJ_VAL(fh);
}


/**
 * file_close(fh)
 * - returns nil on parameter error
 * - returns true on success... yes, this can fail.  Don't ask how.
 */
Value cc_function_fh_close(int arg_count, Value* args) {
    if(arg_count != 1 || !IS_FILEHANDLE(args[0])) {
        return FERROR_VAL(FE_ARG_1_FH);
    }

    ObjFileHandle* fh = AS_FILEHANDLE(args[0]);
    // The close automatically flushes, but let's do that *before* the unlock.
    if(fh->is_writer && fflush(fh->handle) != 0) {
        return FERROR_AUTOERRNO_VAL(FE_FCLOSE_FLUSH_FAILED);
    }
    // Don't try to double-release a lock.
    if(fh->lock.l_type != F_UNLCK) {
        // Because POSIX locking is region based, it's best to just change the lock
        // type in the original lock data and pass it back through.
        fh->lock.l_type = F_UNLCK;
        if(fcntl(fileno(fh->handle), F_SETLK, &fh->lock) != 0) {
            return FERROR_AUTOERRNO_VAL(FE_FCLOSE_UNFLOCK_FAILED);
        }
    }
    // Now we can finally actually close the handle.  Yes, it's safe to use
    // fclose() here instead of close().
    if(fclose(fh->handle) != 0) {
        return FERROR_AUTOERRNO_VAL(FE_FCLOSE_FCLOSE_FAILED_LOL);
    }
    fh->is_open = false;
    return BOOL_VAL(true);
}


/**
 * file_read_line(fh)
 * - returns nil on parameter error
 * - returns false on end of file
 * - returns the string next line otherwise, including the newline
 */
Value cc_function_fh_read_line(int arg_count, Value* args) {
    if(arg_count != 1 || !IS_FILEHANDLE(args[0])) {
        return FERROR_VAL(FE_ARG_1_FH);
    }
    ObjFileHandle* fh = AS_FILEHANDLE(args[0]);

    // Don't even bother if we've reached EOF, if the file isn't open, or it's
    // not open for reading.
    if(feof(fh->handle) != 0 || !fh->is_reader || !fh->is_open) {
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
        int old_errno = errno;
        free(buffer);
        if(feof(fh->handle) != 0) {
            return BOOL_VAL(false);
        }
        return FERROR_ERRNO_VAL(FE_FREAD_GETLINE_FAILED, old_errno);
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
Value cc_function_fh_at_eof(int arg_count, Value* args) {
    if(arg_count != 1 || !IS_FILEHANDLE(args[0])) {
        return FERROR_VAL(FE_ARG_1_FH);
    }
    if(!AS_FILEHANDLE(args[0])->is_open) {
        return BOOL_VAL(false);
    }

    return BOOL_VAL(feof(AS_FILEHANDLE(args[0])->handle) != 0);
}


/**
 * file_write(fh, data)
 * - returns nil on error
 * - returns the number of bytes written
 */
Value cc_function_fh_write(int arg_count, Value* args) {
    if(arg_count != 2) { return FERROR_VAL(FE_ARG_COUNT_2); }
    if(!IS_FILEHANDLE(args[0])) { return FERROR_VAL(FE_ARG_1_FH); }
    if(!IS_STRING(args[1])) { return FERROR_VAL(FE_ARG_2_STRING); }

    ObjFileHandle* fh = AS_FILEHANDLE(args[0]);
    if(!fh->is_writer|| !fh->is_open) {
        return BOOL_VAL(false);
    }

    int res = fputs(AS_CSTRING(args[1]), fh->handle);
    if(res == EOF) {
        return FERROR_AUTOERRNO_VAL(FE_FWRITE_FPUTS_FAILED);
    }
    return NUMBER_VAL(AS_STRING(args[1])->length);
}


/**
 * file_read_block(fh, max_length?)
 * - returns nil on error
 * - returns false on end of file
 * - returns a string containing up to max_length bytes
 *   - if max_length is missing or zero, the entire file will be read
 */
Value cc_function_fh_read_block(int arg_count, Value* args) { return NIL_VAL; }


/**
 * file_truncate(fh, length = 0)
 * - returns nil on error
 * - returns true on success
 */
Value cc_function_fh_truncate(int arg_count, Value* args) { return NIL_VAL; }


/**
 * file_position(fh)
 * - returns nil on error
 * - returns the position of the current filehandle within the file, in bytes
 */
Value cc_function_fh_position(int arg_count, Value* args) { return NIL_VAL; }


/**
 * file_seek(fh, relative_position)
 * - returns nil on error
 * - returns true on success
 */
Value cc_function_fh_seek(int arg_count, Value* args) { return NIL_VAL; }


/**
 * file_seek_start(fh, offset?)
 * - returns nil on error
 * - returns true on success
 * - moves the file pointer to the start of the file, plus the given offset
 */
Value cc_function_fh_seek_start(int arg_count, Value* args) { return NIL_VAL; }


/**
 * file_seek_end(fh, offset?)
 * - returns nil on error
 * - returns true on success
 * - moves the file pointer to the end of the file, plus the given offset
 */
Value cc_function_fh_seek_end(int arg_count, Value* args) { return NIL_VAL; }


Value cc_function_dir_get_all(int arg_count, Value* args) {
    if(arg_count != 1) { return FERROR_VAL(FE_ARG_COUNT_1); }
    if(!IS_STRING(args[0])) { return FERROR_VAL(FE_ARG_1_STRING); }

    DIR* dh = opendir(AS_CSTRING(args[0]));
    if(dh == NULL) {
        return FERROR_AUTOERRNO_VAL(FE_DIR_DIROPEN_FAILED);
    }

    struct dirent* entry = NULL;
    ObjUserArray* ua = newUserArray();

    int index = 0;
    do {
        errno = 0;
        if((entry = readdir(dh)) != NULL) {
            int entlen = strlen(entry->d_name);
            if(entlen == 1 && strncmp(entry->d_name, ".", 1) == 0) {
                continue;
            }
            if(entlen == 2 && strncmp(entry->d_name, "..", 2) == 0) {
                continue;
            }
            ua_grow(ua, index);
            ua->inner.values[index++] = OBJ_VAL(copyString(entry->d_name, entlen));
        }
    } while(entry != NULL);
    ua->inner.count = index;

    if(errno) {
        return FERROR_AUTOERRNO_VAL(FE_DIR_READDIR_FAILED);
    }

    closedir(dh);

    return OBJ_VAL(ua);
}


Value cc_function_file_resolve_path(int arg_count, Value* args) {
    if(arg_count != 1) { return FERROR_VAL(FE_ARG_COUNT_1); }
    if(!IS_STRING(args[0])) { return FERROR_VAL(FE_ARG_1_STRING); }

    ObjString* filename = AS_STRING(args[0]);

    char* resolved = ALLOCATE(char, PATH_MAX);
    realpath(filename->chars, resolved);
    if(resolved == NULL) {
        return FERROR_AUTOERRNO_VAL(FE_FILE_REALPATH_FAILED);
    }
    ObjString* fp = copyString(resolved, strlen(resolved));
    FREE(char, resolved);
    return OBJ_VAL(fp);
}


Value cc_function_file_is_directory(int arg_count, Value* args) {
    if(arg_count != 1) { return FERROR_VAL(FE_ARG_COUNT_1); }
    if(!IS_STRING(args[0])) { return FERROR_VAL(FE_ARG_1_STRING); }

    ObjString* filename = AS_STRING(args[0]);

    struct stat* status = NULL;
    if(stat(filename->chars, status) == 0) {
        return BOOL_VAL(S_ISDIR(status->st_mode) != 0);
    }
    return FERROR_AUTOERRNO_VAL(FE_FILE_STAT_FAILED);
}


Value cc_function_file_is_regular(int arg_count, Value* args) {
    if(arg_count != 1) { return FERROR_VAL(FE_ARG_COUNT_1); }
    if(!IS_STRING(args[0])) { return FERROR_VAL(FE_ARG_1_STRING); }

    ObjString* filename = AS_STRING(args[0]);

    struct stat* status = NULL;
    if(stat(filename->chars, status) == 0) {
        return BOOL_VAL(S_ISREG(status->st_mode) != 0);
    }
    return FERROR_AUTOERRNO_VAL(FE_FILE_STAT_FAILED);
}


Value cc_function_file_is_symlink(int arg_count, Value* args) {
    if(arg_count != 1) { return FERROR_VAL(FE_ARG_COUNT_1); }
    if(!IS_STRING(args[0])) { return FERROR_VAL(FE_ARG_1_STRING); }

    ObjString* filename = AS_STRING(args[0]);

    struct stat* status = NULL;
    if(stat(filename->chars, status) == 0) {
        return BOOL_VAL(S_ISLNK(status->st_mode) != 0);
    }
    return FERROR_AUTOERRNO_VAL(FE_FILE_STAT_FAILED);
}


Value cc_function_file_is_special(int arg_count, Value* args) {
    if(arg_count != 1) { return FERROR_VAL(FE_ARG_COUNT_1); }
    if(!IS_STRING(args[0])) { return FERROR_VAL(FE_ARG_1_STRING); }

    ObjString* filename = AS_STRING(args[0]);

    struct stat* status = NULL;
    if(stat(filename->chars, status) == 0) {
        if(S_ISDIR(status->st_mode) == 0
           && S_ISREG(status->st_mode) == 0
           && S_ISLNK(status->st_mode) == 0
        ) {
            // If it's not a directory, a regular file, or a symlink, it must
            // be some other type of file.
            return BOOL_VAL(true);
        }
        return BOOL_VAL(false);
    }
    return FERROR_AUTOERRNO_VAL(FE_FILE_STAT_FAILED);
}



void cc_register_ext_file() {
    defineNative("file_exists",         cc_function_file_exists);
    defineNative("file_open",           cc_function_file_open);

    defineNative("fh_close",            cc_function_fh_close);
    defineNative("fh_read_line",        cc_function_fh_read_line);
    defineNative("fh_at_eof",           cc_function_fh_at_eof);
    defineNative("fh_write",            cc_function_fh_write);
    defineNative("fh_read_block",       cc_function_fh_read_block);
    defineNative("fh_truncate",         cc_function_fh_truncate);
    defineNative("fh_position",         cc_function_fh_position);
    defineNative("fh_seek",             cc_function_fh_seek);
    defineNative("fh_seek_start",       cc_function_fh_seek_start);
    defineNative("fh_seek_end",         cc_function_fh_seek_end);

    defineNative("dir_get_all",         cc_function_dir_get_all);
    defineNative("file_resolve_path",   cc_function_file_resolve_path);
    defineNative("file_is_directory",   cc_function_file_is_directory);
    defineNative("file_is_regular",     cc_function_file_is_regular);
    defineNative("file_is_symlink",     cc_function_file_is_symlink);
    defineNative("file_is_special",     cc_function_file_is_special);
}
