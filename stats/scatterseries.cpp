// SPDX-License-Identifier: GPL-2.0
#include "scatterseries.h"
#include "informationbox.h"
#include "statscolors.h"
#include "statshelper.h"
#include "statstranslations.h"
#include "statsvariables.h"
#include "zvalues.h"
#include "core/dive.h"
#include "core/divelist.h"
#include "core/qthelper.h"

#include <QGraphicsPixmapItem>
#include <QPainter>

static const int scatterItemDiameter = 10;
static const int scatterItemBorder = 1;

ScatterSeries::ScatterSeries(QGraphicsScene *scene, StatsAxis *xAxis, StatsAxis *yAxis,
			     const StatsVariable &varX, const StatsVariable &varY) :
	StatsSeries(scene, xAxis, yAxis),
	varX(varX), varY(varY)
{
}

ScatterSeries::~ScatterSeries()
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
		scatterPixmapPtr.reset(new QPixmap(createScatterPixmap(fillColor, ::borderColor)));
		scatterPixmapHighlightedPtr.reset(new QPixmap(createScatterPixmap(highlightedColor, highlightedBorderColor)));
	}
	return highlight ? *scatterPixmapHighlightedPtr : *scatterPixmapPtr;
}

ScatterSeries::Item::Item(QGraphicsScene *scene, ScatterSeries *series, dive *d, double pos, double value) :
	item(createItemPtr<QGraphicsPixmapItem>(scene, scatterPixmap(false))),
	d(d),
	pos(pos),
	value(value)
{
	item->setZValue(ZValues::series);
	updatePosition(series);
}

void ScatterSeries::Item::updatePosition(ScatterSeries *series)
{
	QPointF center = series->toScreen(QPointF(pos, value));
	item->setPos(center.x() - scatterItemDiameter / 2.0,
		     center.y() - scatterItemDiameter / 2.0);
}

void ScatterSeries::Item::highlight(bool highlight)
{
	item->setPixmap(scatterPixmap(highlight));
}

void ScatterSeries::append(dive *d, double pos, double value)
{
	items.emplace_back(scene, this, d, pos, value);
}

void ScatterSeries::updatePositions()
{
	for (Item &item: items)
		item.updatePosition(this);
}

static double sq(double f)
{
	return f * f;
}

static double squareDist(const QPointF &p1, const QPointF &p2)
{
	QPointF diff = p1 - p2;
	return QPointF::dotProduct(diff, diff);
}

std::vector<int> ScatterSeries::getItemsUnderMouse(const QPointF &point) const
{
	std::vector<int> res;
	double x = point.x();

	auto low = std::lower_bound(items.begin(), items.end(), x - scatterItemDiameter,
				    [] (const Item &item, double x) { return item.item->pos().x() < x; });
	auto high = std::upper_bound(low, items.end(), x + scatterItemDiameter,
				    [] (double x, const Item &item) { return x < item.item->pos().x(); });
	// Hopefully that narrows it down enough. For discrete scatter plots, we could also partition
	// by equal x and do a binary search in these partitions. But that's probably not worth it.
	res.reserve(high - low);
	double minSquare = sq(scatterItemDiameter / 2.0 + scatterItemBorder);
	for (auto it = low; it < high; ++it) {
		QPointF pos = it->item->pos();
		pos.rx() += scatterItemDiameter / 2.0 + scatterItemBorder;
		pos.ry() += scatterItemDiameter / 2.0 + scatterItemBorder;
		if (squareDist(pos, point) <= minSquare)
			res.push_back(it - items.begin());
	}
	return res;
}

static QString dataInfo(const StatsVariable &var, const dive *d)
{
	// For "numeric" variables, we display value and unit.
	// For "discrete" variables, we display all categories the dive belongs to.
	// There is only one "continuous" variable, the date, for which we don't display anything,
	// because the date is displayed anyway.
	QString val;
	switch (var.type()) {
	case StatsVariable::Type::Numeric:
		val = var.valueWithUnit(d);
		break;
	case StatsVariable::Type::Discrete:
		val = var.diveCategories(d);
		break;
	default:
		return QString();
	}

	return QString("%1: %2").arg(var.name(), val);
}

// Highlight item when hovering over item
bool ScatterSeries::hover(QPointF pos)
{
	std::vector<int> newHighlighted = getItemsUnderMouse(pos);

	if (newHighlighted == highlighted) {
		if (information)
			information->setPos(pos);
		return !newHighlighted.empty();
	}

	// This might be overkill: differential unhighlighting / highlighting of items.
	for (int idx: highlighted) {
		if (std::find(newHighlighted.begin(), newHighlighted.end(), idx) == newHighlighted.end())
			items[idx].highlight(false);
	}
	for (int idx: newHighlighted) {
		if (std::find(highlighted.begin(), highlighted.end(), idx) == highlighted.end())
			items[idx].highlight(true);
	}
	highlighted = std::move(newHighlighted);

	if (highlighted.empty()) {
		information.reset();
		return false;
	} else {
		if (!information)
			information = createItemPtr<InformationBox>(scene);

		std::vector<QString> text;
		text.reserve(highlighted.size() * 5);
		int show = information->recommendedMaxLines() / 5;
		int shown = 0;
		for (int idx: highlighted) {
			const dive *d = items[idx].d;
			if (!text.empty())
				text.push_back(QString(" ")); // Argh. Empty strings are filtered away.
			// We don't listen to undo-command signals, therefore we have to check whether that dive actually exists!
			// TODO: fix this.
			if (get_divenr(d) < 0) {
				text.push_back(StatsTranslations::tr("Removed dive"));
			} else {
				text.push_back(StatsTranslations::tr("Dive #%1").arg(d->number));
				text.push_back(get_dive_date_string(d->when));
				text.push_back(dataInfo(varX, d));
				text.push_back(dataInfo(varY, d));
			}
			if (++shown >= show && shown < (int)highlighted.size()) {
				text.push_back(" ");
				text.push_back(StatsTranslations::tr("and %1 more").arg((int)highlighted.size() - shown));
				break;
			}
		}

		information->setText(text, pos);
		return true;
	}
}

void ScatterSeries::unhighlight()
{
	for (int idx: highlighted)
		items[idx].highlight(false);
	highlighted.clear();
}
