// SPDX-License-Identifier: GPL-2.0
// Color constants for the various series
#ifndef STATSCOLORS_H
#define STATSCOLORS_H

#include <QColor>

inline const QColor fillColor(0x44, 0x76, 0xaa);
inline const QColor borderColor(0x66, 0xb2, 0xff);
inline const QColor highlightedColor(Qt::yellow);
inline const QColor highlightedBorderColor(0xaa, 0xaa, 0x22);
inline const QColor labelColor(Qt::black);

// Colors created using the Chroma.js Color Palette Helper
// https://vis4.net/palettes/#/14|d|00108c,3ed8ff,ffffe0|ffffe0,ff005e,743535|1|1
inline const QColor binColors[] = {
	QRgb(0x00108c), QRgb(0x2737a0), QRgb(0x3f59b4), QRgb(0x567bc6), QRgb(0x719ed5), QRgb(0x93c0e1), QRgb(0xbfe2e7),
	QRgb(0xffdac4), QRgb(0xffb2a6), QRgb(0xf68d8b), QRgb(0xe26c72), QRgb(0xc74f5c), QRgb(0xa33c47), QRgb(0x743535)
};
inline QColor binColor(int bin, int numBins)
{
	if (numBins == 1 || bin < 0 || bin >= numBins)
		return fillColor;
	else
		return binColors[bin % std::size(binColors)];
}

#endif
