// SPDX-License-Identifier: GPL-2.0
// Helper functions for range manipulations
#ifndef RANGE_H
#define RANGE_H

// Move a range in a vector to a different position.
// The parameters are given according to the usual STL-semantics:
//	v: a container with STL-like random access iterator via std::begin(...)
//	rangeBegin: index of first element
//	rangeEnd: index one *past* last element
//	destination: index to element before which the range will be moved
// Owing to std::begin() magic, this function works with STL-like containers:
//	QVector<int> v{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
//	move_in_range(v, 1, 4, 6);
// as well as with C-style arrays:
//	int array[10] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
//	move_in_range(array, 1, 4, 6);
// Both calls will have the following effect:
//	Before: 0 1 2 3 4 5 6 7 8 9
//	After:  0 4 5 1 2 3 6 7 8 9
// No sanitizing of the input arguments is performed.
template <typename Range>
void move_in_range(Range &v, int rangeBegin, int rangeEnd, int destination)
{
	auto it = std::begin(v);
	if (destination > rangeEnd)
		std::rotate(it + rangeBegin, it + rangeEnd, it + destination);
	else if (destination < rangeBegin)
		std::rotate(it + destination, it + rangeBegin, it + rangeEnd);
}

#endif
