#ifndef clox_common_h
#define clox_common_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef DEBUG
#define DEBUG_PRINT_CODE
#define DEBUG_TRACE_EXECUTION
#endif

// Turning off this flag so that my IDE sees these as defined.
//#xifdef CC_FEATURES
#define FEATURE_ECHO
#define FEATURE_EXIT
#define FEATURE_FUNCTIONS
#define FEATURE_STRING_BACKSLASH_ESCAPES
#define FEATURE_STRING_SINGLE_QUOTED
//#xendif

#define UINT8_COUNT (UINT8_MAX + 1)

#endif
