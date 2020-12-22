// SPDX-License-Identifier: GPL-2.0
// Color constants for the various series
#ifndef STATSCOLORS_H
#define STATSCOLORS_H

#include <QColor>

inline const QColor fillColor(0x44, 0x76, 0xaa);
inline const QColor borderColor(0x66, 0xb2, 0xff);
inline const QColor highlightedColor(Qt::yellow);
inline const QColor highlightedBorderColor(0xaa, 0xaa, 0x22);

// Colors taken from Qt's QtChart module, which is licensed under GPL
// Original file: qtcharts/src/charts/themes/chartthemelight_p.h
inline const QColor binColors[] = {
        fillColor, QRgb(0x209fdf), QRgb(0x99ca53), QRgb(0xf6a625), QRgb(0x6d5fd5), QRgb(0xbf593e)
};
inline QColor binColor(int bin)
{
	return bin < 0 ? binColors[0] : binColors[bin % std::size(binColors)];
}

#endif
