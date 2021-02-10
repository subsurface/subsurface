// SPDX-License-Identifier: GPL-2.0
/*
 * metrics.h
 *
 * header file for common function to find/compute essential UI metrics
 * (font properties, icon sizes, etc)
 *
 */
#ifndef METRICS_H
#define METRICS_H

#include <QFont>
#include <QFontMetrics>
#include <QSize>

QFont defaultModelFont();
QFontMetrics defaultModelFontMetrics();

// Collection of icon/picture sizes and other metrics, resolution independent
struct IconMetrics {
	// icon sizes
	int sz_small; // ex 16px
	int sz_med; // ex 24px
	int sz_big; // ex 32px
	// picture size
	int sz_pic; // ex 128px
	// icon spacing
	int spacing; // ex 2px
	// devicePixelRatio
	double dpr; // 1.0 for traditional screens, HiDPI screens up to 3.0
	IconMetrics();
};

const IconMetrics &defaultIconMetrics();
void updateDevicePixelRatio(double dpr);

#endif // METRICS_H
