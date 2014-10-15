/*
 * metrics.cpp
 *
 * methods to find/compute essential UI metrics
 * (font properties, icon sizes, etc)
 *
 */

#include "metrics.h"

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
int defaultIconSize(int height)
{
	int ret = (height + 8)/16;
	ret *= 16;
	if (ret < 16)
		ret = 16;
	return ret;
}
