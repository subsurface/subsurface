// SPDX-License-Identifier: GPL-2.0
// Color constants for the various series
#ifndef STATSCOLORS_H
#define STATSCOLORS_H

#include <QColor>

inline const QColor backgroundColor(Qt::white);
inline const QColor fillColor(0x44, 0x76, 0xaa);
inline const QColor borderColor(0x66, 0xb2, 0xff);
inline const QColor selectedColor(0xaa, 0x76, 0x44);
inline const QColor selectedBorderColor(0xff, 0xb2, 0x66);
inline const QColor highlightedColor(Qt::yellow);
inline const QColor highlightedBorderColor(0xaa, 0xaa, 0x22);
inline const QColor darkLabelColor(Qt::black);
inline const QColor lightLabelColor(Qt::white);
inline const QColor axisColor(Qt::black);
inline const QColor gridColor(0xcc, 0xcc, 0xcc);
inline const QColor informationBorderColor(Qt::black);
inline const QColor informationColor(0xff, 0xff, 0x00, 192); // Note: fourth argument is opacity
inline const QColor legendColor(0x00, 0x8e, 0xcc, 192); // Note: fourth argument is opacity
inline const QColor legendBorderColor(Qt::black);
inline const QColor quartileMarkerColor(Qt::red);
inline const QColor regressionItemColor(Qt::red);
inline const QColor meanMarkerColor(Qt::green);
inline const QColor medianMarkerColor(Qt::red);
inline const QColor selectionLassoColor(Qt::black);
inline const QColor selectionOverlayColor(Qt::lightGray);

QColor binColor(int bin, int numBins);
QColor labelColor(int bin, size_t numBins);

#endif
