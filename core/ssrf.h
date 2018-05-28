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

#ifdef SUBSURFACE_MOBILE
#ifdef ENABLE_STARTUP_TIMING
// Declare generic function, will be seen only in CPP code
// Use void parameters to avoid extra includes
extern void log_stp(const char *ident, QString *buf);

#define LOG_STP(x) log_stp(x, NULL)
#define LOG_STP_CLIPBOARD(x) log_stp(NULL, x)
#else
#define LOG_STP(x)
#define LOG_STP_CLIPBOARD(x)
#endif // ENABLE_STARTUP_TIMING
#endif // SUBSURFACE_MOBILE

}
#else

// Macro to be used for silencing unused parameters
#define UNUSED(x) (void)x
#endif

#endif // SSRF_H
