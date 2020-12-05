// SPDX-License-Identifier: GPL-2.0
// A small custom box plot series, which displays quartiles.
// The QtCharts bar series were  too inflexible with respect
// to placement of the bars.
#ifndef BOX_SERIES_H
#define BOX_SERIES_H

#include "statstypes.h" // for StatsQuartiles

#include <memory>
#include <vector>
#include <QScatterSeries>
#include <QGraphicsLineItem>
#include <QGraphicsRectItem>

namespace QtCharts {
	class QAbstractAxis;
	class QChart;
}
class InformationBox;
class StatsType;

// We derive from a proper scatter series to get access to the map-to
// and map-from coordinates calls. But we don't use any of its functionality.
// Can we avoid that?

class BoxSeries : public QtCharts::QScatterSeries {
public:
	BoxSeries(const QString &variable, const QString &unit, int decimals);
	~BoxSeries();

	// Call if chart geometry changed
	void updatePositions();

	// Note: this expects that all items are added with increasing pos
	// and that no bar is inside another bar, i.e. lowerBound and upperBound
	// are ordered identically.
	void append(double lowerBound, double upperBound, const StatsQuartiles &q, const QString &binName);

	// Get item under mous pointer, or -1 if none
	int getItemUnderMouse(const QPointF &f);

	// Highlight item when hovering over item
	void highlight(int index, QPointF pos);
private:
	struct Item {
		QGraphicsRectItem box;
		QGraphicsLineItem topWhisker, bottomWhisker;
		QGraphicsLineItem topBar, bottomBar;
		QGraphicsLineItem center;
		QRectF bounding; // bounding box in screen coordinates
		~Item();
		double lowerBound, upperBound;
		StatsQuartiles q;
		QString binName;
		Item(QtCharts::QChart *chart, BoxSeries *series, double lowerBound, double upperBound, const StatsQuartiles &q, const QString &binName);
		void updatePosition(QtCharts::QChart *chart, BoxSeries *series);
		void highlight(bool highlight);
	};

	QString variable, unit;
	int decimals;

	std::vector<QString> formatInformation(const Item &item) const;
	std::unique_ptr<InformationBox> information;
	std::vector<std::unique_ptr<Item>> items;
	int highlighted; // -1: no item highlighted
};

#endif
