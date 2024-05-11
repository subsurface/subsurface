// SPDX-License-Identifier: GPL-2.0
// Helper functions for range manipulations
#ifndef RANGE_H
#define RANGE_H

#include <utility>	// for std::pair
#include <vector>	// we need a declaration of std::begin() and std::end()

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

// A rudimentary adaptor for looping over ranges with an index:
//	for (auto [idx, item]: enumerated_range(v)) ...
// The index is a signed integer, since this is what we use more often.
template <typename Range>
class enumerated_range
{
	Range &base;
public:
	using base_iterator = decltype(std::begin(std::declval<Range &>()));
	class iterator {
		int idx;
		base_iterator it;
	public:
		std::pair<int, decltype(*it)> operator*() const
		{
			return { idx, *it };
		}
		iterator &operator++()
		{
			++idx;
			++it;
			return *this;
		}
		iterator(int idx, base_iterator it) : idx(idx), it(it)
		{
		}
		bool operator==(const iterator &it2) const
		{
			return it == it2.it;
		}
		bool operator!=(const iterator &it2) const
		{
			return it != it2.it;
		}
	};

	iterator begin()
	{
		return iterator(0, std::begin(base));
	}
	iterator end()
	{
		return iterator(-1, std::end(base));
	}

	enumerated_range(Range &base) : base(base)
	{
	}
};

// Find the index of an element in a range. Return -1 if not found
// Range must have a random access iterator.
template <typename Range, typename Element>
int index_of(const Range &range, const Element &e)
{
	auto it = std::find(std::begin(range), std::end(range), e);
	return it == std::end(range) ? -1 : it - std::begin(range);
}

template <typename Range, typename Func>
int index_of_if(const Range &range, Func f)
{
	auto it = std::find_if(std::begin(range), std::end(range), f);
	return it == std::end(range) ? -1 : it - std::begin(range);
}

// Not really appropriate here, but oh my.
template<typename Range, typename Element>
bool range_contains(const Range &v, const Element &item)
{
	return std::find(v.begin(), v.end(), item) != v.end();
}

#endif
