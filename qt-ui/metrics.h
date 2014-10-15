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

QFont defaultModelFont();
QFontMetrics defaultModelFontMetrics();

// return the default icon size, computed as the multiple of 16 closest to
// the given height (that defaults to the default font height)
int defaultIconSize(int height = defaultModelFontMetrics().height());

#endif // METRICS_H
