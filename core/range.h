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

// Small helper base class for iterator adapters.
template <typename Base>
class iterator_adapter {
protected:
	Base it;
public:
	iterator_adapter(Base it) : it(it)
	{
	}
	bool operator==(const iterator_adapter &it2) const {
		return it == it2.it;
	}
	bool operator!=(const iterator_adapter &it2) const
	{
		return it != it2.it;
	}
};

// A rudimentary adapter for looping over pairs of elements in ranges:
//	for (auto [it1, it2]: pairwise_range(v)) ...
// The pairs are overlapping, i.e. there is one less pair than elements:
// { 1, 2, 3, 4 } -> (1,2), (2,3), (3,4)
template <typename Range>
class pairwise_range
{
	Range &base;
public:
	using base_iterator = decltype(std::begin(std::declval<Range &>()));
	using item_type = decltype(*std::begin(base));
	class iterator : public iterator_adapter<base_iterator> {
	public:
		using iterator_adapter<base_iterator>::iterator_adapter;
		std::pair<item_type &, item_type &> operator*() const
		{
			return { *this->it, *std::next(this->it) };
		}
		iterator &operator++()
		{
			++this->it;
			return *this;
		}
		iterator &operator--()
		{
			--this->it;
			return *this;
		}
		iterator operator++(int)
		{
			return iterator(this->it++);
		}
		iterator operator--(int)
		{
			return iterator(this->it--);
		}
	};

	iterator begin()
	{
		return iterator(std::begin(base));
	}
	iterator end()
	{
		return std::begin(base) == std::end(base) ?
			iterator(std::begin(base)) :
			iterator(std::prev(std::end(base)));
	}

	pairwise_range(Range &base): base(base)
	{
	}
};

// A rudimentary adapter for looping over ranges with an index:
//	for (auto [idx, item]: enumerated_range(v)) ...
// The index is a signed integer, since this is what we use more often.
template <typename Range>
class enumerated_range
{
	Range &base;
public:
	using base_iterator = decltype(std::begin(std::declval<Range &>()));
	class iterator : public iterator_adapter<base_iterator>{
		int idx;
	public:
		using iterator_adapter<base_iterator>::iterator_adapter;
		using item_type = decltype(*std::begin(base));
		std::pair<int, item_type &> operator*() const
		{
			return { idx, *this->it };
		}
		iterator &operator++()
		{
			++idx;
			++this->it;
			return *this;
		}
		iterator &operator--()
		{
			--idx;
			--this->it;
			return *this;
		}
		iterator &operator++(int) = delete; // Postfix increment/decrement not supported for now
		iterator &operator--(int) = delete; // Postfix increment/decrement not supported for now
		iterator(int idx, base_iterator it) : iterator_adapter<base_iterator>(it), idx(idx)
		{
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
	return std::find(std::begin(v), std::end(v), item) != v.end();
}

// Insert into an already sorted range
template<typename Range, typename Element, typename Comp>
void range_insert_sorted(Range &v, Element &item, Comp comp)
{
	auto it = std::lower_bound(std::begin(v), std::end(v), item,
				   [&comp](auto &a, auto &b) { return comp(a, b) < 0; });
	v.insert(it, std::move(item));
}

// Insert into an already sorted range, but don't add an item twice
template<typename Range, typename Element, typename Comp>
void range_insert_sorted_unique(Range &v, Element &item, Comp comp)
{
	auto it = std::lower_bound(std::begin(v), std::end(v), item,
				   [&comp](auto &a, auto &b) { return comp(a, b) < 0; });
	if (it == std::end(v) || comp(item, *it) != 0)
		v.insert(it, std::move(item));
}

template<typename Range, typename Element>
void range_remove(Range &v, const Element &item)
{
	auto it = std::find(std::begin(v), std::end(v), item);
	if (it != std::end(v))
		v.erase(it);
}

#endif
