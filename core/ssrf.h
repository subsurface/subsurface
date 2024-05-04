// SPDX-License-Identifier: GPL-2.0
#ifndef SSRF_H
#define SSRF_H

extern "C" {

#ifdef __clang__
// Clang has a bug on zero-initialization of C structs.
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif

}

// Macro to be used for silencing unused parameters
#define UNUSED(x) (void)x

#endif // SSRF_H
