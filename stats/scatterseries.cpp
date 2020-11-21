// SPDX-License-Identifier: GPL-2.0
#include "scatterseries.h"
#include "statstranslations.h"
#include "statstypes.h"
#include "core/dive.h"
#include "core/divelist.h"
#include "core/qthelper.h"

#include <QChart>
#include <QGraphicsPixmapItem>
#include <QPainter>

static const QColor scatterItemColor(0x66, 0xb2, 0xff);
static const QColor scatterItemBorderColor(Qt::blue);
static const QColor scatterItemHighlightedColor(Qt::yellow);
static const QColor scatterItemHighlightedBorderColor(0xaa, 0xaa, 0x22);
static const int scatterItemDiameter = 10;
static const int scatterItemBorder = 1;
static const QColor informationBorderColor(Qt::black);
static const QColor informationColor(0xff, 0xff, 0x00, 192); // Note: fourth argument is opacity
static const int informationBorder = 2;

ScatterSeries::ScatterSeries(const StatsType &typeX, const StatsType &typeY)
	: highlighted(-1), typeX(typeX), typeY(typeY)
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

ScatterSeries::Item::Item(QtCharts::QChart *chart, ScatterSeries *series, dive *d, double pos, double value) :
	item(new QGraphicsPixmapItem(scatterPixmap(false), chart)),
	d(d),
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

void ScatterSeries::append(dive *d, double pos, double value)
{
	items.emplace_back(chart(), this, d, pos, value);
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
	if (highlighted >= 0 && highlighted < (int)items.size()) {
		Item &item = items[highlighted];
		item.highlight(true);
		QPointF pos = item.item->pos();
		if (!information)
			information.reset(new Information(chart()));
		information->set(items[highlighted].d, pos, typeX, typeY);
	} else {
		information.reset();
	}
}

ScatterSeries::Information::Information(QtCharts::QChart *chart) : QGraphicsRectItem(chart)
{
	setPen(QPen(informationBorderColor, informationBorder));
	setBrush(informationColor);
}

void ScatterSeries::Information::set(const dive *d, QPointF pos, const StatsType &typeX, const StatsType &typeY)
{
	width = height = 0.0;
	textItems.clear();

	// We don't listen to undo-command signals, therefore we have to check whether that dive actually exists!
	// TODO: fix this.
	if (get_divenr(d) < 0) {
		addLine(StatsTranslations::tr("Removed dive"));
	} else {
		addLine(StatsTranslations::tr("Dive #%1").arg(d->number));
		addLine(get_dive_date_string(d->when));
		addDataLine(typeX, d);
		addDataLine(typeY, d);
	}

	width += 4.0 * informationBorder;
	height += 4.0 * informationBorder;

	QRectF parentRect = parentItem()->boundingRect();

	double x = pos.x() + scatterItemDiameter;
	if (x + width >= parentRect.right() && pos.x() - width >= parentRect.x())
		x = pos.x() - width;
	double y = pos.y() + scatterItemDiameter;
	if (y + height >= parentRect.bottom() && pos.y() - height >= parentRect.y())
		y = pos.y() - height;


	setRect(x, y, width, height);
	double actY = y + 2.0 * informationBorder;
	for (auto &item: textItems) {
		item->setPos(QPointF(x + 2.0 * informationBorder, actY));
		actY += item->boundingRect().height();
	}
	setZValue(20.0); // What would a proper value be here?
}

void ScatterSeries::Information::addLine(const QString &s)
{
	textItems.emplace_back(new QGraphicsSimpleTextItem(s, this));
	QGraphicsSimpleTextItem &item = *textItems.back();
	item.setPos(QPointF(0.0, height));
	QRectF rect = item.boundingRect();
	width = std::max(width, rect.width());
	height += rect.height();
}

void ScatterSeries::Information::addDataLine(const StatsType &type, const dive *d)
{
	// For "numeric" types, we display value and unit.
	// For "discrete" types, we display all categories the dive belongs to.
	// There is only one "continuous" type, the date, for which we don't display anything,
	// because the date is displayed anyway.
	QString val;
	switch (type.type()) {
	case StatsType::Type::Numeric:
		val = type.valueWithUnit(d);
		break;
	case StatsType::Type::Discrete:
		val = type.diveCategories(d);
		break;
	default:
		return;
	}

	addLine(QString("%1: %2").arg(type.name(), val));
}
