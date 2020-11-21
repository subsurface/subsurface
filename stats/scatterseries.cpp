// SPDX-License-Identifier: GPL-2.0
#include "scatterseries.h"

#include <QChart>
#include <QGraphicsPixmapItem>
#include <QPainter>

static const QColor scatterItemColor(0x66, 0xb2, 0xff);
static const QColor scatterItemBorderColor(Qt::blue);
static const QColor scatterItemHighlightedColor(Qt::yellow);
static const QColor scatterItemHighlightedBorderColor(0xaa, 0xaa, 0x22);
static const int scatterItemDiameter = 10;
static const int scatterItemBorder = 1;

ScatterSeries::ScatterSeries()
	: highlighted(-1)
{
}

static QPixmap createScatterPixmap(const QColor &color, const QColor &borderColor)
{
	QPixmap res(scatterItemDiameter, scatterItemDiameter);
	res.fill(Qt::transparent);
	QPainter painter(&res);
	painter.setPen(Qt::NoPen);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setBrush(borderColor);
	painter.drawEllipse(0, 0, scatterItemDiameter, scatterItemDiameter);
	painter.setBrush(color);
	painter.drawEllipse(scatterItemBorder, scatterItemBorder,
			    scatterItemDiameter - 2 * scatterItemBorder,
			    scatterItemDiameter - 2 * scatterItemBorder);
	return res;
}

// Annoying: we can create a QPixmap only after the application was initialized.
// Therefore, do this as a on-demand initialized pointer. A function local static
// variable does unnecesssary (in this case) thread synchronization.
static std::unique_ptr<QPixmap> scatterPixmapPtr;
static std::unique_ptr<QPixmap> scatterPixmapHighlightedPtr;

static const QPixmap &scatterPixmap(bool highlight)
{
	if (!scatterPixmapPtr) {
		scatterPixmapPtr.reset(new QPixmap(createScatterPixmap(scatterItemColor,
								       scatterItemBorderColor)));
		scatterPixmapHighlightedPtr.reset(new QPixmap(createScatterPixmap(scatterItemHighlightedColor,
										  scatterItemHighlightedBorderColor)));
	}
	return highlight ? *scatterPixmapHighlightedPtr : *scatterPixmapPtr;
}

ScatterSeries::Item::Item(QtCharts::QChart *chart, ScatterSeries *series, double pos, double value) :
	item(new QGraphicsPixmapItem(scatterPixmap(false), chart)),
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

void ScatterSeries::Item::highlight(bool highlight)
{
	item->setPixmap(scatterPixmap(highlight));
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

static double sq(double f)
{
	return f * f;
}

static double squareDist(const QPointF &p1, const QPointF &p2)
{
	return sq(p1.x() - p2.x()) + sq(p1.y() - p2.y());
}

// Attention: this supposes that items are sorted by x-position!
std::pair<double, int> ScatterSeries::getClosest(const QPointF &point)
{
	double x = point.x();

	auto low = std::lower_bound(items.begin(), items.end(), x - scatterItemDiameter,
				    [] (const Item &item, double x) { return item.item->pos().x() < x; });
	auto high = std::upper_bound(low, items.end(), x + scatterItemDiameter,
				    [] (double x, const Item &item) { return x < item.item->pos().x(); });
	// Hopefully that narrows it down enough. For discrete scatter plots, we could also partition
	// by equal x and do a binary search in these partitions. But that's probably not worth it.
	double minSquare = sq(scatterItemDiameter); // Only consider items within twice the radius
	int index = -1;
	for (auto it = low; it < high; ++it) {
		QPointF pos = it->item->pos();
		pos.rx() += scatterItemDiameter / 2.0;
		pos.ry() += scatterItemDiameter / 2.0;
		double square = squareDist(pos, point);
		if (square <= minSquare) {
			index = it - items.begin();
			minSquare = square;
		}
	}
	return { minSquare, index };
}

// Highlight item when hovering over item
void ScatterSeries::highlight(int index)
{
	if (index == highlighted)
		return;
	// Unhighlight old highlighted item (if any)
	if (highlighted >= 0 && highlighted < (int)items.size())
		items[highlighted].highlight(false);
	highlighted = index;
	// Highlight new item (if any)
	if (highlighted >= 0 && highlighted < (int)items.size())
		items[highlighted].highlight(true);
}
