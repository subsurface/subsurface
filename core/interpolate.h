// SPDX-License-Identifier: GPL-2.0
#ifndef INTERPOLATE_H
#define INTERPOLATE_H

#include <type_traits>
#include <math.h> // lrint()

/* Linear interpolation between 'a' and 'b', when we are 'part'way into the 'whole' distance from a to b
 * Since we're currently stuck with C++17, we have this horrible way of testing whether the template
 * parameter is an integral type.
 */
template <typename T,
	std::enable_if_t<std::is_integral<T>::value, bool> = true>
T interpolate(T a, T b, int part, int whole)
{
	/* It is doubtful that we actually need floating point for this, but whatever */
	if (whole) {
		double x = (double)a * (whole - part) + (double)b * part;
		return (int)lrint(x / whole);
	}
	return (a+b)/2;
}

#endif
