// SPDX-License-Identifier: GPL-2.0
/*
 * We use singletons in numerous places. In combination with QML this gives
 * us a very fundamental problem, because QML likes to allocate objects by
 * itself. There are known workarounds, but currently we use idiosyncratic
 * singleton classes, which initialize the global instance pointer in the
 * constructor. Things get even more complicated if the same singleton is used
 * on mobile and on desktop. The latter might want to simply use the classical
 * instance() function without having to initialize the singleton first, as
 * that would beat the purpose of the instance() method.
 *
 * The template defined here, SillySingleton, codifies all this. Simply derive
 * a class from this template:
 *	class X : public SillySingleton<X> {
 *		...
 *	};
 * This will generate an instance() method. This will do the right thing for
 * both methods: explicit construction of the singleton via new or implicit
 * by calling the instance() method. It will also generate warnings if a
 * singleton class is generated more than once (i.e. first instance() is called
 * and _then_ new).
 *
 * In the long run we should get rid of all users of this class.
 */
#ifndef SINGLETON_H
#define SINGLETON_H

#include <typeinfo>
#include <QtGlobal>

// 1) Declaration
template<typename T>
class SillySingleton {
	static T *self;
protected:
	SillySingleton();
	~SillySingleton();
public:
	static T *instance();
};

template<typename T>
T *SillySingleton<T>::self = nullptr;

// 2) Implementation

template<typename T>
SillySingleton<T>::SillySingleton()
{
	if (self)
		qWarning("Generating second instance of singleton %s", typeid(T).name());
	self = static_cast<T *>(this);
	qDebug("Generated singleton %s", typeid(T).name());
}

template<typename T>
SillySingleton<T>::~SillySingleton()
{
	if (self == this)
		self = nullptr;
	else
		qWarning("Destroying unknown instance of singleton %s", typeid(T).name());
	qDebug("Destroyed singleton %s", typeid(T).name());
}

template<typename T>
T *SillySingleton<T>::instance()
{
	if (!self)
		new T;
	return self;
}

#endif
