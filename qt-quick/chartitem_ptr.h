// SPDX-License-Identifier: GPL-2.0
// A stupid pointer class that initializes to null and can be copy
// assigned. This is for historical reasons: unique_ptrs to ChartItems
// were replaced by plain pointers. Instead of nulling the plain pointers
// in the constructors, use this. Ultimately, we might think about making
// this thing smarter, once removal of individual ChartItems is implemented.
#ifndef CHARITEM_PTR_H
#define CHARITEM_PTR_H

template <typename T>
class ChartItemPtr {
	friend class ChartView; // Only the chart view can create these pointers
	T *ptr;
	ChartItemPtr(T *ptr) : ptr(ptr)
	{
	}
public:
	ChartItemPtr() : ptr(nullptr)
	{
	}
	ChartItemPtr(const ChartItemPtr &p) : ptr(p.ptr)
	{
	}
	ChartItemPtr(ChartItemPtr &&p) : ptr(p.ptr)
	{
	}
	void reset()
	{
		ptr = nullptr;
	}
	ChartItemPtr &operator=(const ChartItemPtr &p)
	{
		ptr = p.ptr;
		return *this;
	}
	operator bool() const
	{
		return !!ptr;
	}
	bool operator!() const
	{
		return !ptr;
	}
	T &operator*() const
	{
		return *ptr;
	}
	T *operator->() const
	{
		return ptr;
	}
};

#endif
