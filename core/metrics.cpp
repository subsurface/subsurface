// SPDX-License-Identifier: GPL-2.0
/*
 * metrics.cpp
 *
 * methods to find/compute essential UI metrics
 * (font properties, icon sizes, etc)
 *
 */

#include "metrics.h"

static IconMetrics dfltIconMetrics;

IconMetrics::IconMetrics() :
	sz_small(-1),
	sz_med(-1),
	sz_big(-1),
	sz_pic(-1),
	spacing(-1),
	dpr(1.0)
{
}

QFont defaultModelFont()
{
	QFont font;
	return font;
}

QFontMetrics defaultModelFontMetrics()
{
	return QFontMetrics(defaultModelFont());
}

// return the default icon size, computed as the multiple of 16 closest to
// the given height
static int defaultIconSize(int height)
{
	int ret = (height + 8)/16;
	ret *= 16;
	if (ret < 16)
		ret = 16;
	return ret;
}

const IconMetrics &defaultIconMetrics()
{
	if (dfltIconMetrics.sz_small == -1) {
		int small = defaultIconSize(defaultModelFontMetrics().height());
		dfltIconMetrics.sz_small = small;
		dfltIconMetrics.sz_med = small + small/2;
		dfltIconMetrics.sz_big = 2*small;

		dfltIconMetrics.sz_pic = 8*small;

		dfltIconMetrics.spacing = small/8;
	}

	return dfltIconMetrics;
}

void updateDevicePixelRatio(double dpr)
{
	dfltIconMetrics.dpr = dpr;
}
