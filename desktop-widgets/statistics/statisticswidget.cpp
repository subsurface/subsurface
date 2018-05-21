// SPDX-License-Identifier: GPL-2.0
#include "desktop-widgets/statistics/statisticswidget.h"
#include "qt-models/yearlystatisticsmodel.h"
#include <QModelIndex>

YearlyStatisticsWidget::YearlyStatisticsWidget(QWidget *parent):
	QGraphicsView(parent),
	m_model(NULL)
{
}

void YearlyStatisticsWidget::setModel(YearlyStatisticsModel *m)
{
	m_model = m;
	connect(m, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
			this, SLOT(modelDataChanged(QModelIndex,QModelIndex)));
	connect(m, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)),
			scene(), SLOT(clear()));
	connect(m, SIGNAL(rowsInserted(QModelIndex,int,int)),
			this, SLOT(modelRowsInserted(QModelIndex,int,int)));

	modelRowsInserted(QModelIndex(),0,m_model->rowCount()-1);
}

void YearlyStatisticsWidget::modelRowsInserted(const QModelIndex&, int, int)
{
	// stub
}

void YearlyStatisticsWidget::modelDataChanged(const QModelIndex&, const QModelIndex&)
{
	scene()->clear();
	modelRowsInserted(QModelIndex(),0,m_model->rowCount()-1);
}

void YearlyStatisticsWidget::resizeEvent(QResizeEvent *event)
{
	QGraphicsView::resizeEvent(event);
	fitInView(sceneRect(), Qt::IgnoreAspectRatio);
}
