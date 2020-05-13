#include "../common.h"
#include "../vm.h"


/**
 * file_open(filename, mode)
 * - returns nil on error
 * - returns filehandle on success
 * - mode may be:
 *   - "r" to read from the file, or fail if it does not exist
 *   - "w" to write to the file, creating it if it does not exist
 *     - **NOTE** "w" does *not* truncate the file!
 *   - "rw" to read and write from the file, or fail if it does not exist
 * - "r" mode establishes a shared lock on the file.
 * - the "w" modes establish an exclusive lock on the file
 * - files are opened in "binary" mode
 */
Value cc_function_file_open(int arg_count, Value* args) { return NIL_VAL; }


/**
 * file_close(fh)
 * - returns nil on error
 * - returns true on success
 */
Value cc_function_file_close(int arg_count, Value* args) { return NIL_VAL; }


/**
 * file_read_line(fh, newline = '\n')
 * - returns nil on error
 * - returns false on end of file
 * - returns the string next line otherwise, including the newline/delimiter
 * - reminder: files are opened in binary mode, so no newline interpolation is done
 */
Value cc_function_file_read_line(int arg_count, Value* args) { return NIL_VAL; }


/**
 * file_at_eof(fh)
 * - returns nil on error
 * - returns true if the end of the file has been reached
 */
Value cc_function_file_at_eof(int arg_count, Value* args) { return NIL_VAL; }


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
