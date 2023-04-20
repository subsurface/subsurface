#ifndef UNITS_OPERATORS_H
#define UNITS_OPERATORS_H

#ifdef __cplusplus

#include <utility>

template <typename T>
struct VectorType {
private:
	T &downcast() { return static_cast<T&>(*this); }
	const T &downcast() const { return static_cast<const T&>(*this); }
	auto base() const { auto [y] = downcast(); return y; }
	// Miscompiles on some clang versions!
	//auto &base() { auto &[y] = downcast(); return y; }
public:
	VectorType() { auto &[y] = downcast(); y = 0; }
	VectorType(int x) { auto &[y] = downcast(); y = x; }
	T &operator+=(T v) { auto &[y] = downcast(); y += v.base(); return downcast(); }
	T &operator-=(T v) { auto &[y] = downcast(); y  -= v.base(); return downcast(); }
	T operator+(T v) const { T res = downcast(); res += v; return res; }
	T operator-(T v) const { T res = downcast(); res -= v; return res; }
	T operator-() const { T res = { -base() }; return res; }
	T &operator*=(int x) { auto &[y] = downcast(); y *= x; return downcast(); }
	T &operator/=(int x) { auto &[y] = downcast(); y /= x; return downcast(); }
	T operator*(int x) const { T res = downcast(); res *= x; return res; }
	T operator/(int x) const { T res = downcast(); res /= x; return res; }

	// C++20 would be a single line here:
	//auto operator<=>(const VectorType<T> &v) const { return base() <=> v.base(); }
	bool operator==(T v) const { return base() == v.base(); }
	bool operator!=(T v) const { return base() != v.base(); }
	bool operator<(T v) const { return base() < v.base(); }
	bool operator>(T v) const { return base() > v.base(); }
	bool operator<=(T v) const { return base() <= v.base(); }
	bool operator>=(T v) const { return base() >= v.base(); }
};

#endif

#endif
