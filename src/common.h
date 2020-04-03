#ifndef clox_common_h
#define clox_common_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// clangd doesn't think null exists, so let's macro it.
#ifndef NULL
#define NULL 0
#endif

// Nor does it think that size_t exists.  The standard definition seems to be
// a sixteen-bit unsigned integer, so let's fake it as that.
#ifndef size_t
#define size_t uint16_t
#endif

#endif
