// SPDX-License-Identifier: GPL-2.0
// Defines the z-values of features in the profile view.
// Objects with higher z-values are painted on top of objects
// with smaller z-values. For the same z-value objects are
// drawn in order of addition to the scene.
#ifndef PROFILE_ZVALUES_H
#define PROFILE_ZVALUES_H

// Encapsulating an enum in a struct is stupid, but allows us
// to not poison the namespace and yet autoconvert to int
// (in constrast to enum class). enum is so broken!
struct ProfileZValue {
	enum ZValues {
		Profile = 0,
		ToolTipItem,
		Count
	};
};

#endif
