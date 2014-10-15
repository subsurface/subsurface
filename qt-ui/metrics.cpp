/*
 * metrics.cpp
 *
 * methods to find/compute essential UI metrics
 * (font properties, icon sizes, etc)
 *
 */

#include "metrics.h"

static IconMetrics dfltIconMetrics = { -1 };

QFont defaultModelFont()
{
	QFont font;
//	font.setPointSizeF(font.pointSizeF() * 0.8);
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

const IconMetrics & defaultIconMetrics()
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
