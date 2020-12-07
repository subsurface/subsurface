// SPDX-License-Identifier: GPL-2.0
// Supported chart axes
#ifndef STATS_AXIS_H
#define STATS_AXIS_H

#include <vector>
#include <QBarCategoryAxis>
#include <QCategoryAxis>
#include <QValueAxis>

namespace QtCharts {
	class QChart;
}

class StatsAxis {
public:
	virtual ~StatsAxis();
	virtual void updateLabels(const QtCharts::QChart *chart) = 0;
	virtual QtCharts::QAbstractAxis *qaxis() = 0;
protected:
	StatsAxis(bool horizontal);
	int guessNumTicks(const QtCharts::QChart *chart, const QtCharts::QAbstractAxis *axis, const std::vector<QString> &strings) const;
	bool horizontal;
};

// Small template that derives from a QChart-axis and defines
// the corresponding virtual axis() accessor.
template<typename QAxis>
class StatsAxisTemplate : public StatsAxis, public QAxis
{
	using StatsAxis::StatsAxis;
	QtCharts::QAbstractAxis *qaxis() override final {
		return this;
	}
};

class ValueAxis : public StatsAxisTemplate<QtCharts::QValueAxis> {
public:
	ValueAxis(double min, double max, int decimals, bool horizontal);
private:
	double min, max;
	int decimals;
	void updateLabels(const QtCharts::QChart *chart) override;
};

class CountAxis : public StatsAxisTemplate<QtCharts::QValueAxis> {
public:
	CountAxis(int count, bool horizontal);
private:
	int count;
	void updateLabels(const QtCharts::QChart *chart) override;
};

class CategoryAxis : public StatsAxisTemplate<QtCharts::QBarCategoryAxis> {
public:
	CategoryAxis(const std::vector<QString> &labels, bool horizontal);
private:
	void updateLabels(const QtCharts::QChart *chart);
};

struct HistogramAxisEntry {
	QString name;
	double value;
	bool recommended;
};

class HistogramAxis : public StatsAxisTemplate<QtCharts::QCategoryAxis> {
public:
	HistogramAxis(std::vector<HistogramAxisEntry> bin_values, bool horizontal);
private:
	void updateLabels(const QtCharts::QChart *chart) override;
	std::vector<HistogramAxisEntry> bin_values;
};

#endif
