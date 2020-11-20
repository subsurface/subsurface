// SPDX-License-Identifier: GPL-2.0
#include "scatterseries.h"

#include <QChart>
#include <QGraphicsPixmapItem>
#include <QPainter>

static const QColor scatterItemColor(0x66, 0xb2, 0xff);
static const QColor scatterItemBorderColor(Qt::blue);
static const int scatterItemDiameter = 10;
static const int scatterItemBorder = 1;

static QPixmap createScatterPixmap()
{
	QPixmap res(scatterItemDiameter, scatterItemDiameter);
	res.fill(Qt::transparent);
	QPainter painter(&res);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setBrush(scatterItemBorderColor);
	painter.drawEllipse(0, 0, scatterItemDiameter, scatterItemDiameter);
	painter.setBrush(scatterItemColor);
	painter.drawEllipse(scatterItemBorder, scatterItemBorder,
			    scatterItemDiameter - 2 * scatterItemBorder,
			    scatterItemDiameter - 2 * scatterItemBorder);
	return res;
}

// Annoying: we can create a QPixmap only after the application was initialized.
// Therefore, do this as a on-demand initialized pointer. A function local static
// variable does unnecesssary (in this case) thread synchronization.
static std::unique_ptr<QPixmap> scatterPixmapPtr;

static const QPixmap &scatterPixmap()
{
	if (!scatterPixmapPtr)
		scatterPixmapPtr.reset(new QPixmap(createScatterPixmap()));
	return *scatterPixmapPtr;
}

ScatterSeries::Item::Item(QtCharts::QChart *chart, ScatterSeries *series, double pos, double value) :
	item(new QGraphicsPixmapItem(scatterPixmap(), chart)),
	pos(pos),
	value(value)
{
	item->setZValue(5.0); // ? What is a sensible value here ?
	updatePosition(chart, series);
}

void ScatterSeries::Item::updatePosition(QtCharts::QChart *chart, ScatterSeries *series)
{
	QPointF center = chart->mapToPosition(QPointF(pos, value), series);
	item->setPos(center.x() - scatterItemDiameter / 2.0,
		     center.y() - scatterItemDiameter / 2.0);
}

void ScatterSeries::append(double pos, double value)
{
	items.emplace_back(chart(), this, pos, value);
}

void ScatterSeries::updatePositions()
{
	QtCharts::QChart *c = chart();
	for (Item &item: items)
		item.updatePosition(c, this);
}
