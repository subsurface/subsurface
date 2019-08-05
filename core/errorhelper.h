// SPDX-License-Identifier: GPL-2.0
#ifndef ERROR_HELPER_H
#define ERROR_HELPER_H

// error reporting functions

#ifdef __cplusplus
extern "C" {
#endif

extern int verbose;
extern int report_error(const char *fmt, ...);
extern void set_error_cb(void(*cb)(char *));	// Callback takes ownership of passed string

#ifdef __cplusplus
}
#endif

#endif
