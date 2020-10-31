// SPDX-License-Identifier: GPL-2.0
#ifndef STATSWIDGET_H
#define STATSWIDGET_H

#include "ui_statswidget.h"

class StatsWidget : public QWidget {
	Q_OBJECT
public:
	StatsWidget(QWidget *parent = 0);
private
slots:
	void closeStats();
	void chartTypeChanged(int);
	void firstAxisChanged(int);
	void secondAxisChanged(int);
	void plot();
private:
	Ui::StatsWidget ui;
};

#endif // STATSWIDGET_H
