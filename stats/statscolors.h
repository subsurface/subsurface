// SPDX-License-Identifier: GPL-2.0
// Color constants for the various series
#ifndef STATSCOLORS_H
#define STATSCOLORS_H

#include <QColor>

inline const QColor fillColor(0x44, 0x76, 0xaa);
inline const QColor borderColor(0x66, 0xb2, 0xff);
inline const QColor highlightedColor(Qt::yellow);
inline const QColor highlightedBorderColor(0xaa, 0xaa, 0x22);
inline const QColor darkLabelColor(Qt::black);
inline const QColor lightLabelColor(Qt::white);

QColor binColor(int bin, int numBins);
QColor labelColor(int bin, size_t numBins);

#endif
