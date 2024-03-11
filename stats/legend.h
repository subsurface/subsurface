// SPDX-License-Identifier: GPL-2.0
// A legend box, which is shown on the chart.
#ifndef STATS_LEGEND_H
#define STATS_LEGEND_H

#include "qt-quick/chartitem.h"

#include <memory>
#include <vector>

class QFontMetrics;
class StatsTheme;

class Legend : public ChartRectItem {
public:
	Legend(ChartView &view, const StatsTheme &theme, const std::vector<QString> &names);
	void resize(); // called when the chart size changes.
	void setPos(QPointF pos); // Attention: not virtual - always call on this class.
private:
	// Each entry is a text besides a rectangle showing the color
	struct Entry {
		QString name;
		QBrush rectBrush;
		QPointF pos;
		double width;
		Entry(const QString &name, int idx, int numBins, const QFontMetrics &fm, const StatsTheme &);
	};
	int displayedItems;
	double width;
	double height;
	const StatsTheme &theme; // Set once in constructor.
	// The position is specified with respect to the center and in relative terms
	// with respect to the canvas.
	QPointF centerPos;
	bool posInitialized;
	int fontHeight;
	std::vector<Entry> entries;
	void hide();
};

#endif
