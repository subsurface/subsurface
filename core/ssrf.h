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

#ifdef ENABLE_STARTUP_TIMING
#ifdef STARTUP_TIMER
#define STP_SETUP() QTime stpDuration; \
					QString stpText
#define STP_RUN() stpDuration.start()
// Block to help determine where time is "lost" in
// the mobile version during startup
#include <QTime>
extern QTime stpDuration;
extern QString stpText;
#define LOG_STP(x) stpText +=QString("STP ") \
							.append(QString::number(stpDuration.elapsed())) \
							.append(" ms, ") \
							.append(x) \
							.append("\n")
#endif // STARTUP_TIMER
#else // Release version
#define STP_SETUP() 
#define STP_RUN()
#define LOG_STP(x)
#endif

#endif // SSRF_H
