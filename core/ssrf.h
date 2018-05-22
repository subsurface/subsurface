// SPDX-License-Identifier: GPL-2.0
#ifndef SSRF_H
#define SSRF_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __clang__
// Clang has a bug on zero-initialization of C structs.
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif

#ifdef __cplusplus
}
#else

// Macro to be used for silencing unused parameters
#define UNUSED(x) (void)x
#endif

#endif // SSRF_H
