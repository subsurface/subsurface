// SPDX-License-Identifier: GPL-2.0
#include "legend.h"
#include "statscolors.h"
#include "zvalues.h"

#include <QFontMetrics>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QPen>

static const double legendBorderSize = 2.0;
static const double legendBoxBorderSize = 1.0;
static const double legendBoxBorderRadius = 4.0;	// radius of rounded corners
static const double legendBoxScale = 0.8;		// 1.0: text-height of the used font
static const double legendInternalBorderSize = 2.0;
static const QColor legendColor(0x00, 0x8e, 0xcc, 192); // Note: fourth argument is opacity
static const QColor legendBorderColor(Qt::black);

Legend::Legend(const std::vector<QString> &names) :
	RoundRectItem(legendBoxBorderRadius),
	displayedItems(0), width(0.0), height(0.0)
{
	setZValue(ZValues::legend);
	entries.reserve(names.size());
	int idx = 0;
	for (const QString &name: names)
		entries.emplace_back(name, idx++, (int)names.size(), this);

	// Calculate the height and width of the elements
	if (!entries.empty()) {
		QFontMetrics fm(entries[0].text->font());
		fontHeight = fm.height();
		for (Entry &e: entries)
			e.width = fontHeight + 2.0 * legendBoxBorderSize +
				  fm.size(Qt::TextSingleLine, e.text->text()).width();
	} else {
		// Set to an arbitrary non-zero value, because Coverity doesn't understand
		// that we don't use the value as divisor below if entries is empty.
		fontHeight = 10.0;
	}
	setPen(QPen(legendBorderColor, legendBorderSize));
	setBrush(QBrush(legendColor));
}

Legend::Entry::Entry(const QString &name, int idx, int numBins, QGraphicsItem *parent) :
	rect(new QGraphicsRectItem(parent)),
	text(new QGraphicsSimpleTextItem(name, parent)),
	width(0)
{
	rect->setZValue(ZValues::legend);
	rect->setPen(QPen(legendBorderColor, legendBoxBorderSize));
	rect->setBrush(QBrush(binColor(idx, numBins)));
	text->setZValue(ZValues::legend);
	text->setBrush(QBrush(darkLabelColor));
}

void Legend::hide()
{
	for (Entry &e: entries) {
		e.rect->hide();
		e.text->hide();
	}
	QGraphicsRectItem::hide();
}

void Legend::resize()
{
	if (entries.empty())
		return hide();

	QSizeF size = scene()->sceneRect().size();

	// Silly heuristics: make the legend at most half as high and half as wide as the chart.
	// Not sure if that makes sense - this might need some optimization.
	int maxRows = static_cast<int>(size.height() / 2.0 - 2.0 * legendInternalBorderSize) / fontHeight;
	if (maxRows <= 0)
		return hide();
	int numColumns = ((int)entries.size() - 1) / maxRows + 1;
	int numRows = ((int)entries.size() - 1) / numColumns + 1;

	double x = legendInternalBorderSize;
	displayedItems = 0;
	for (int col = 0; col < numColumns; ++col) {
		double y = legendInternalBorderSize;
		double nextX = x;

		for (int row = 0; row < numRows; ++row) {
			int idx = col * numRows + row;
			if (idx >= (int)entries.size())
				break;
			entries[idx].pos = QPointF(x, y);
			nextX = std::max(nextX, x + entries[idx].width);
			y += fontHeight;
			++displayedItems;
		}
		x = nextX;
		width = nextX;
		if (width >= size.width() / 2.0) // More than half the chart-width -> give up
			break;
	}
	width += legendInternalBorderSize;
	height = 2 * legendInternalBorderSize + numRows * fontHeight;
	updatePosition();
}

void Legend::updatePosition()
{
	if (displayedItems <= 0)
		return hide();
	// For now, place the legend in the top right corner.
	QPointF pos(scene()->sceneRect().width() - width - 10.0, 10.0);
	setRect(QRectF(pos, QSizeF(width, height)));
	for (int i = 0; i < displayedItems; ++i) {
		QPointF itemPos = pos + entries[i].pos;
		QRectF rect(itemPos, QSizeF(fontHeight, fontHeight));
		// Decrease box size by legendBoxScale factor
		double delta = fontHeight * (1.0 - legendBoxScale) / 2.0;
		rect = rect.adjusted(delta, delta, -delta, -delta);
		entries[i].rect->setRect(rect);
		itemPos.rx() += fontHeight + 2.0 * legendBoxBorderSize;
		entries[i].text->setPos(itemPos);
		entries[i].rect->show();
		entries[i].text->show();
	}
	for (int i = displayedItems; i < (int)entries.size(); ++i) {
		entries[i].rect->hide();
		entries[i].text->hide();
	}
	show();
}
