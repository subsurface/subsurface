// SPDX-License-Identifier: GPL-2.0
#ifndef STATS_AXIS_H
#define STATS_AXIS_H

#include "chartitem.h"

#include <memory>
#include <vector>

class ChartLineItem;
class QFontMetrics;

// The labels and the title of the axis are rendered into a pixmap.
// The ticks and the baseline are realized as individual ChartLineItems.
class StatsAxis : public ChartPixmapItem {
public:
	virtual ~StatsAxis();
	// Returns minimum and maximum of shown range, not of data points.
	std::pair<double, double> minMax() const;
	std::pair<double, double> minMaxScreen() const; // minimum and maximum in screen coordinates
	std::pair<double, double> horizontalOverhang() const; // space that labels peak out in horizontal axes

	double width() const;		// Only supported by vertical axes. Only valid after setSize().
	double height() const;		// Only supported for horizontal axes. Always valid.
	void setSize(double size);	// Width for horizontal and height for vertical.
	void setPos(QPointF pos);	// Must be called after setSize().
	void setRange(double, double);

	// Map x (horizontal) or y (vertical) coordinate to or from screen coordinate
	double toScreen(double) const;
	double toValue(double) const;

	std::vector<double> ticksPositions() const; // Positions in screen coordinates
protected:
	StatsAxis(ChartView &view, const StatsTheme &theme, const QString &title, bool horizontal, bool labelsBetweenTicks);

	const StatsTheme &theme; // Initialized once in constructor.
	ChartItemPtr<ChartLineItem> line;
	QString title;
	double titleWidth;

	struct Label {
		QString label;
		int width;
		double pos;
	};
	std::vector<Label> labels;
	void addLabel(const QFontMetrics &fm, const QString &label, double pos);
	virtual void updateLabels() = 0;
	virtual std::pair<QString, QString> getFirstLastLabel() const = 0;

	struct Tick {
		ChartItemPtr<ChartLineItem> item;
		double pos;
	};
	std::vector<Tick> ticks;
	void addTick(double pos);

	int guessNumTicks(const std::vector<QString> &strings) const;
	bool horizontal;
	bool labelsBetweenTicks;	// When labels are between ticks, they can be moved closer to the axis

	double size;			// width for horizontal, height for vertical
	double zeroOnScreen;
	QPointF offset;			// Offset of the label and title pixmap with respect to the (0,0) position.
	double min, max;
	double labelWidth;		// Maximum width of labels
private:
	double titleSpace() const;	// Space needed for title
};

class ValueAxis : public StatsAxis {
public:
	ValueAxis(ChartView &view, const StatsTheme &theme, const QString &title,
		  double min, double max, int decimals, bool horizontal);
private:
	double min, max;
	int decimals;
	void updateLabels() override;
	std::pair<QString, QString> getFirstLastLabel() const override;
};

class CountAxis : public ValueAxis {
public:
	CountAxis(ChartView &view, const StatsTheme &theme, const QString &title, int count, bool horizontal);
private:
	int count;
	void updateLabels() override;
	std::pair<QString, QString> getFirstLastLabel() const override;
};

class CategoryAxis : public StatsAxis {
public:
	CategoryAxis(ChartView &view, const StatsTheme &theme, const QString &title,
		     const std::vector<QString> &labels, bool horizontal);
private:
	std::vector<QString> labelsText;
	void updateLabels();
	std::pair<QString, QString> getFirstLastLabel() const override;
};

struct HistogramAxisEntry {
	QString name;
	double value;
	bool recommended;
};

class HistogramAxis : public StatsAxis {
public:
	HistogramAxis(ChartView &view, const StatsTheme &theme, const QString &title,
		      std::vector<HistogramAxisEntry> bin_values, bool horizontal);
private:
	void updateLabels() override;
	std::pair<QString, QString> getFirstLastLabel() const override;
	std::vector<HistogramAxisEntry> bin_values;
	int preferred_step;
};

class DateAxis : public HistogramAxis {
public:
	DateAxis(ChartView &view, const StatsTheme &theme, const QString &title, double from, double to, bool horizontal);
};

#endif
