// SPDX-License-Identifier: GPL-2.0
#ifndef TAB_DIVE_STATISTICS_H
#define TAB_DIVE_STATISTICS_H

#include "TabBase.h"
#include "core/subsurface-qt/divelistnotifier.h"

namespace Ui {
	class TabDiveStatistics;
};

class TabDiveStatistics : public TabBase {
	Q_OBJECT
public:
	TabDiveStatistics(QWidget *parent = 0);
	~TabDiveStatistics();
	void updateData() override;
	void clear() override;

private slots:
	void divesChanged(const QVector<dive *> &dives, DiveField field);
	void cylinderChanged(dive *d);

private:
	Ui::TabDiveStatistics *ui;
};

// Widget describing, minimum, maximum and average value.
// Scheduled for removal in due course.
class QLabel;
class MinMaxAvgWidget : public QWidget {
	Q_OBJECT
	QLabel *avgIco, *avgValue;
	QLabel *minIco, *minValue;
	QLabel *maxIco, *maxValue;
public:
	MinMaxAvgWidget(QWidget *parent);
	double minimum() const;
	double maximum() const;
	double average() const;
	void setMinimum(double minimum);
	void setMaximum(double maximum);
	void setAverage(double average);
	void setMinimum(const QString &minimum);
	void setMaximum(const QString &maximum);
	void setAverage(const QString &average);
	void overrideMinToolTipText(const QString &newTip);
	void overrideAvgToolTipText(const QString &newTip);
	void overrideMaxToolTipText(const QString &newTip);
	void setAvgVisibility(bool visible);
	void clear();
};

#endif
