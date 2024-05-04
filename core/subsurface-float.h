// SPDX-License-Identifier: GPL-2.0
#ifndef SUBSURFACE_FLOAT_H
#define SUBSURFACE_FLOAT_H

#include <math.h>

static inline bool nearly_equal(double a, double b)
{
	return fabs(a - b) <= 1e-6 * fmax(fabs(a), fabs(b));
}

static inline bool nearly_0(double fp)
{
	return fabs(fp) <= 1e-6;
}

#endif // SUBSURFACE_FLOAT_H
