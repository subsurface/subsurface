// SPDX-License-Identifier: GPL-2.0
#ifndef STATS_AXIS_H
#define STATS_AXIS_H

#include <memory>
#include <vector>
#include <QBarCategoryAxis>
#include <QCategoryAxis>
#include <QFont>
#include <QGraphicsSimpleTextItem>
#include <QGraphicsLineItem>
#include <QValueAxis>

namespace QtCharts {
	class QChart;
}

class StatsAxis : QGraphicsLineItem {
public:
	virtual ~StatsAxis();
	// Returns minimum and maximum of shown range, not of data points.
	std::pair<double, double> minMax() const;

	double width() const;		// Only supported by vertical axes. Only valid after setSize().
	double height() const;		// Only supported for horizontal axes. Always valid.
	void setSize(double size);	// Width for horizontal and height for vertical.
	void setPos(QPointF pos);	// Must be called after setSize().
	void setRange(double, double);

	// Map x (horizontal) or y (vertical) coordinate to or from screen coordinate
	double toScreen(double) const;
	double toValue(double) const;
protected:
	StatsAxis(QtCharts::QChart *chart, const QString &title, bool horizontal, bool labelsBetweenTicks);
	QtCharts::QChart *chart;

	struct Label {
		std::unique_ptr<QGraphicsSimpleTextItem> label;
		double pos;
		Label(const QString &name, double pos, QtCharts::QChart *chart, const QFont &font);
	};
	std::vector<Label> labels;
	void addLabel(const QString &label, double pos);
	virtual void updateLabels() = 0;

	struct Tick {
		std::unique_ptr<QGraphicsLineItem> item;
		double pos;
		Tick(double pos, QtCharts::QChart *chart);
	};
	std::vector<Tick> ticks;
	void addTick(double pos);

	int guessNumTicks(const std::vector<QString> &strings) const;
	bool horizontal;
	bool labelsBetweenTicks;	// When labels are between ticks, they can be moved closer to the axis

	QFont labelFont, titleFont;
	std::unique_ptr<QGraphicsSimpleTextItem> title;
	double size;			// width for horizontal, height for vertical
	double zeroOnScreen;
	double min, max;
	double labelWidth;		// Maximum width of labels
private:
	double titleSpace() const;	// Space needed for title
};

class ValueAxis : public StatsAxis {
public:
	ValueAxis(QtCharts::QChart *chart, const QString &title, double min, double max, int decimals, bool horizontal);
private:
	double min, max;
	int decimals;
	void updateLabels() override;
};

class CountAxis : public ValueAxis {
public:
	CountAxis(QtCharts::QChart *chart, const QString &title, int count, bool horizontal);
private:
	int count;
	void updateLabels() override;
};

class CategoryAxis : public StatsAxis {
public:
	CategoryAxis(QtCharts::QChart *chart, const QString &title, const std::vector<QString> &labels, bool horizontal);
private:
	void updateLabels();
};

struct HistogramAxisEntry {
	QString name;
	double value;
	bool recommended;
};

class HistogramAxis : public StatsAxis {
public:
	HistogramAxis(QtCharts::QChart *chart, const QString &title, std::vector<HistogramAxisEntry> bin_values, bool horizontal);
private:
	void updateLabels() override;
	std::vector<HistogramAxisEntry> bin_values;
	int preferred_step;
};

class DateAxis : public HistogramAxis {
public:
	DateAxis(QtCharts::QChart *chart, const QString &title, double from, double to, bool horizontal);
};

#endif
