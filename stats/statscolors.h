// SPDX-License-Identifier: GPL-2.0
// Color constants for the various series
#ifndef STATSCOLORS_H
#define STATSCOLORS_H

#include <memory>
#include <QColor>
#include <QFont>
#include <QString>

class QSGTexture;

class StatsTheme {
public:
	StatsTheme();
	virtual QString name() const = 0;
	QColor backgroundColor;
	QColor fillColor;
	QColor borderColor;
	QColor selectedColor;
	QColor selectedBorderColor;
	QColor highlightedColor;
	QColor highlightedBorderColor;
	// The dark and light label colors are with respect to the light theme.
	// In the dark theme, dark is light and light is dark. Might want to change the names!
	QColor darkLabelColor;
	QColor lightLabelColor;
	QColor axisColor;
	QColor gridColor;
	QColor informationBorderColor;
	QColor informationColor;
	QColor legendColor;
	QColor legendBorderColor;
	QColor quartileMarkerColor;
	QColor regressionItemColor;
	QColor meanMarkerColor;
	QColor medianMarkerColor;
	QColor selectionLassoColor;
	QColor selectionOverlayColor;
	virtual QColor binColor(int bin, int numBins) const = 0;
	virtual QColor labelColor(int bin, size_t numBins) const = 0;

	QFont axisLabelFont;
	QFont axisTitleFont;
	QFont informationBoxFont;
	QFont labelFont;
	QFont legendFont;
	QFont titleFont;

	// These are cashes for the QSG rendering engine
	// Note: Originally these were std::unique_ptrs, which automatically
	// freed the textures on exit. However, destroying textures after
	// QApplication finished its thread leads to crashes. Therefore, these
	// are now normal pointers and the texture objects are leaked.
	mutable QSGTexture *scatterItemTexture = nullptr;
	mutable QSGTexture *scatterItemSelectedTexture = nullptr;
	mutable QSGTexture *scatterItemHighlightedTexture = nullptr;
	mutable QSGTexture *selectedTexture = nullptr; // A checkerboard pattern.
};

extern const StatsTheme &getStatsTheme(bool dark);

#endif
