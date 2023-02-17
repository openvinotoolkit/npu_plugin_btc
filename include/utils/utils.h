//
// Copyright (C) 2023 Intel Corporation
// SPDX-License-Identifier: Apache 2.0
//

//

#ifndef __BC_UTILS_HPP__
#define __BC_UTILS_HPP__

#define __STDC_WANT_LIB_EXT1__ 1

#include <errno.h>
#include <cstdlib>
#include <cstdint>
#include <cstring>

#ifndef RSIZE_MAX
#define RSIZE_MAX SIZE_MAX
#endif

#define UNUSED(x) ((void) x)

#ifdef __STDC_LIB_EXT1__
#define memset_safe memset_s
#else
inline static int memset_safe(void* dest, size_t destsz, const uint8_t byte, size_t count) {
    if (dest == NULL) return EINVAL; // dest should not be a NULL ptr
    if (destsz > RSIZE_MAX) return ERANGE;
    if (count > RSIZE_MAX) return ERANGE;
    if (destsz < count) { memset(dest, byte, destsz); return ERANGE; }

    memset(dest, byte, count);

    return 0;
}
#endif

#ifdef __STDC_LIB_EXT1__
#define memcpy_safe memcpy_s
#else
inline static int memcpy_safe(void* dest, size_t destsz, const void* const src, size_t count)
{
    if (dest == NULL) return EINVAL; // dest should not be a NULL ptr
    if (destsz > RSIZE_MAX) return ERANGE;
    if (count > RSIZE_MAX) return ERANGE;
    if (destsz < count) { memset_safe(dest, destsz, 0, destsz); return ERANGE; }
    if (src == NULL) { memset_safe(dest, destsz, 0, destsz); return EINVAL; } // src should not be a NULL ptr

    // Copying shall not take place between regions that overlap.
    if (((dest > src) && (dest < (static_cast<const uint8_t*>(src) + count))) ||
        ((src > dest) && (src < (static_cast<uint8_t*>(dest) + destsz)))) {
        memset_safe(dest, destsz, 0, destsz);
        return ERANGE;
    }

    memcpy(dest, src, count);

    return 0;
}
#endif

#ifdef __STDC_LIB_EXT1__
#define strcpy_safe strcpy_s
#else
inline static int strcpy_safe(char* dest, const size_t dest_size, const char* src) {
    return memcpy_safe(dest, dest_size, src, strlen(src) + 1);
}
#endif

#endif //__BC_UTILS_HPP__
