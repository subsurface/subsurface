// SPDX-License-Identifier: GPL-2.0
// Defines the z-values of features in the chart view.
// Objects with higher z-values are painted on top of objects
// with smaller z-values. For the same z-value objects are
// drawn in order of addition to the graphics scene.
#ifndef ZVALUES_H

struct ZValues {
	static constexpr double axes = 0.0;
	static constexpr double series = 11.0;
	static constexpr double seriesLabels = 12.0;
	static constexpr double chartFeatures = 13.0;	// quartile markers and regression lines
	static constexpr double informationBox = 14.0;
	static constexpr double legend = 15.0;
};

#endif
