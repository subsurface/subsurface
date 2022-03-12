// SPDX-License-Identifier: GPL-2.0
// Collection of objects that will be deleted on application exit.
// This feature is needed because many Qt-objects crash if freed
// after the application has exited.
#ifndef GLOBALS_H
#define GLOBALS_H

#include <memory>
#include <utility>

template <typename T, class... Args>
T *make_global(Args &&...args); // construct a global object of type T.

template <typename T>
T *register_global(T *); // register an already constructed object. returns input.

void free_globals(); // call on application exit. frees all global objects.

// Implementation

// A class with a virtual destructor that will be used to destruct the objects.
struct GlobalObjectBase
{
	virtual ~GlobalObjectBase() { }
};

template <typename T>
struct GlobalObject : T, GlobalObjectBase
{
	using T::T; // Inherit constructor from actual object.
};

void register_global_internal(GlobalObjectBase *);

template <typename T, class... Args>
T *make_global(Args &&...args)
{
	GlobalObject<T> *res = new GlobalObject<T>(std::forward<Args>(args)...);
	register_global_internal(res);
	return res;
}

template <typename T>
T *register_global(T *o)
{
	make_global<std::unique_ptr<T>>(o);
	return o;
}

#endif
