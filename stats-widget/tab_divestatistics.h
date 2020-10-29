// SPDX-License-Identifier: GPL-2.0
#ifndef TAB_DIVE_STATISTICS_H
#define TAB_DIVE_STATISTICS_H

#include "desktop-widgets/tab-widgets/TabBase.h"
#include "core/subsurface-qt/divelistnotifier.h"
#include <QGraphicsScene>
#include <QtCharts>

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
	void on_createGraphButton_clicked();
private:
	Ui::TabDiveStatistics *ui;
//	QGraphicsScene *scene;
//	QGraphicsRectItem *rectangle;
	const QUrl urlHistogramWidget= QUrl(QStringLiteral("qrc:/qml/histogram.qml"));
	const QUrl urlXygraphWidget= QUrl(QStringLiteral("qrc:/qml/xygraph.qml"));

};

#endif
