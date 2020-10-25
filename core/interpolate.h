// SPDX-License-Identifier: GPL-2.0
#ifndef INTERPOLATE_H
#define INTERPOLATE_H

/* Linear interpolation between 'a' and 'b', when we are 'part'way into the 'whole' distance from a to b */
static inline int interpolate(int a, int b, int part, int whole)
{
	/* It is doubtful that we actually need floating point for this, but whatever */
	if (whole) {
		double x = (double)a * (whole - part) + (double)b * part;
		return (int)lrint(x / whole);
	}
	return (a+b)/2;
}

#endif
