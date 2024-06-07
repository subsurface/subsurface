// SPDX-License-Identifier: GPL-2.0
/* Traditionally, most of our data structures are collected as tables
 * (=arrays) of pointers. The advantage is that pointers (or references)
 * to objects are stable, even if the array grows and reallocates.
 * Implicitly, the table is the owner of the objects and the objets
 * are deleted (freed) when removed from the table.
 * This header defines an std::vector<> of unique_ptr<>s to make this
 * explicit.
 *
 * The state I want to reach across the code base: whenever a part of the
 * code owns a heap allocated object, it *always* possesses a unique_ptr<>
 * to that object. All naked pointers are invariably considered as
 * "non owning".
 *
 * There are two ways to end ownership:
 * 1) The std::unique_ptr<> goes out of scope and the object is
 *    automatically deleted.
 * 2) Ownership is passed to another std::unique_ptr<> using std::move().
 *
 * This means that when adding an object to an owning_table,
 * ownership of a std::unique_ptr<> is given up with std::move().
 * The table then returns a non-owning pointer to the object and
 * optionally the insertion index.
 *
 * In converse, when removing an object, one provides a non-owning
 * pointer, which is turned into an owning std::unique_ptr<> and
 * the index where the object was removed.
 * When ignoring the returned owning pointer, the object is
 * automatically freed.
 *
 * The functions to add an entry to the table are called "put()",
 * potentially with a suffix. The functions to remove an entry
 * are called "pull()", likewise with an optional suffix.
 *
 * There are two versions of the table:
 * 1) An unordered version, where the caller is responsible for
 *    adding at specified positions (either given by an index or at the end).
 *    Removal via a non-owning pointer is implemented by a linear search
 *    over the whole table.
 * 2) An ordered version, where a comparison function that returns -1, 0, 1
 *    is used to add objects. In that case, the caller must make sure that
 *    no two ojects that compare equal are added to the table.
 *    Obviously, the compare function is supposed to be replaced by the
 *    "spaceship operator" in due course.
 *    Here, adding and removing via non-owning pointers is implemented
 *    using a binary search.
 *
 * Note that, since the table contains std::unique_ptr<>s, to loop over
 * all entries, it is best to use something such as
 *      for (const auto &ptr: table) ...
 * I plan to add iterator adapters, that autometically dereference
 * the unique_ptr<>s and provide const-references for const-tables.
 *
 * Time will tell how useful this class is.
 */
#ifndef CORE_OWNING_TABLE_H
#define CORE_OWNING_TABLE_H

#include "errorhelper.h"

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

template <typename T>
class owning_table : public std::vector<std::unique_ptr<T>> {
public:
	struct put_result {
		T *ptr;
		size_t idx;
	};
	struct pull_result {
		std::unique_ptr<T> ptr;
		size_t idx;
	};
	size_t get_idx(const T *item) const {
		auto it = std::find_if(this->begin(), this->end(),
				[item] (auto &i1) {return i1.get() == item; });
		return it != this->end() ? it - this->begin() : std::string::npos;
	}
	T *put_at(std::unique_ptr<T> item, size_t idx) {
		T *res = item.get();
		insert(this->begin() + idx, std::move(item));
		return res;
	}
	// Returns index of added item
	put_result put_back(std::unique_ptr<T> item) {
		T *ptr = item.get();
		push_back(std::move(item));
		return { ptr, this->size() - 1 };
	}
	std::unique_ptr<T> pull_at(size_t idx) {
		auto it = this->begin() + idx;
		std::unique_ptr<T> res = std::move(*it);
		this->erase(it);
		return res;
	}
	pull_result pull(const T *item) {
		size_t idx = get_idx(item);
		if (idx == std::string::npos) {
			report_info("Warning: removing unexisting item in %s", __func__);
			return { std::unique_ptr<T>(), std::string::npos };
		}
		return { pull_at(idx), idx };
	}
};

// Note: there must not be any elements that compare equal!
template <typename T, int (*CMP)(const T &, const T &)>
class sorted_owning_table : public owning_table<T> {
public:
	using typename owning_table<T>::put_result;
	using typename owning_table<T>::pull_result;
	// Returns index of added item
	put_result put(std::unique_ptr<T> item) {
		auto it = std::lower_bound(this->begin(), this->end(), item,
				[] (const auto &i1, const auto &i2)
				{ return CMP(*i1, *i2) < 0; });
		if (it != this->end() && CMP(**it, *item) == 0)
			report_info("Warning: adding duplicate item in %s", __func__);
		size_t idx = it - this->begin();
		T *ptr = item.get();
		this->insert(it, std::move(item));
		return { ptr, idx };
	}

	// Optimized version of get_idx(), which uses binary search
	// If not found, fall back to linear search and emit a warning.
	// Note: this is probaly slower than a linesr search. But for now,
	// it helps finding consistency problems.
	size_t get_idx(const T *item) const {
		auto it = std::lower_bound(this->begin(), this->end(), item,
				[] (const auto &i1, const auto &i2)
				{ return CMP(*i1, *i2) < 0; });
		if (it == this->end() || CMP(**it, *item) != 0) {
			size_t idx = owning_table<T>::get_idx(item);
			if (idx != std::string::npos)
				report_info("Warning: index found by linear but not by binary search in %s", __func__);
			return idx;
		}
		return it - this->begin();
	}

	// Get place where insertion would take place
	size_t get_insertion_index(const T *item) const {
		auto it = std::lower_bound(this->begin(), this->end(), item,
				[] (const auto &i1, const auto &i2)
				{ return CMP(*i1, *i2) < 0; });
		return it - this->begin();
	}

	// Note: this is silly - finding the pointer by a linear search
	// is probably significantly faster than doing a binary search.
	// But it helps finding consistency problems for now. Remove in
	// due course.
	pull_result pull(const T *item) {
		size_t idx = get_idx(item);
		if (idx == std::string::npos) {
			report_info("Warning: removing unexisting item in %s", __func__);
			return { std::unique_ptr<T>(), std::string::npos };
		}
		return { this->pull_at(idx), idx };
	}

	void sort() {
		std::sort(this->begin(), this->end(), [](const auto &a, const auto &b) { return CMP(*a, *b) < 0; });
	}
};

#endif
