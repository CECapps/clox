#ifndef clox_common_h
#define clox_common_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef DEBUG
#define DEBUG_PRINT_CODE
#define DEBUG_TRACE_EXECUTION
#define DEBUG_COMPILE_TRACE
#define DEBUG_PRINT_CODE
#endif

#define CC_FEATURES

int global_argc;
const char** global_argv;

#define UINT8_COUNT (UINT8_MAX + 1)

#endif
