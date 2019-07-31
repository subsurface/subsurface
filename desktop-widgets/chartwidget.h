#ifndef CHARTWIDGET_H
#define CHARTWIDGET_H

#include <QWidget>
#include "ui_chartwidget.h"
#include "qt-models/datestatsmodel.h"

namespace Ui {
class ChartWidget;
}

class ChartWidget : public QWidget
{
	Q_OBJECT

public:
	explicit ChartWidget(QWidget *parent = nullptr);
	void DrawYearChart();
	void DrawMonthChart();

private:
	Ui::ChartWidget ui;
	DateStatsTableModel model;
};

#endif // STATSCHARTWIDGET_H
