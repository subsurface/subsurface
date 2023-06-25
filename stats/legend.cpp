// SPDX-License-Identifier: GPL-2.0
#include "legend.h"
#include "statscolors.h"
#include "statsview.h"
#include "zvalues.h"

#include <cmath>
#include <QFontMetrics>
#include <QPen>

static const double legendBorderSize = 2.0;
static const double legendBoxBorderSize = 1.0;
static const double legendBoxBorderRadius = 4.0;	// radius of rounded corners
static const double legendBoxScale = 0.8;		// 1.0: text-height of the used font
static const double legendInternalBorderSize = 2.0;

Legend::Legend(ChartView &view, const StatsTheme &theme, const std::vector<QString> &names) :
	ChartRectItem(view, ChartZValue::Legend,
		      QPen(theme.legendBorderColor, legendBorderSize),
		      QBrush(theme.legendColor), legendBoxBorderRadius,
		      true),
	displayedItems(0), width(0.0), height(0.0),
	theme(theme),
	posInitialized(false)
{
	entries.reserve(names.size());
	QFontMetrics fm(theme.legendFont);
	fontHeight = fm.height();
	int idx = 0;
	for (const QString &name: names)
		entries.emplace_back(name, idx++, (int)names.size(), fm, theme);
}

Legend::Entry::Entry(const QString &name, int idx, int numBins, const QFontMetrics &fm, const StatsTheme &theme) :
	name(name),
	rectBrush(QBrush(theme.binColor(idx, numBins)))
{
	width = fm.height() + 2.0 * legendBoxBorderSize + fm.size(Qt::TextSingleLine, name).width();
}

void Legend::hide()
{
	ChartRectItem::resize(QSizeF(1,1));
	img->fill(Qt::transparent);
}

void Legend::resize()
{
	if (entries.empty())
		return hide();

	QSizeF size = sceneSize();

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

	ChartRectItem::resize(QSizeF(width, height));

	// Paint rectangles
	painter->setPen(QPen(theme.legendBorderColor, legendBoxBorderSize));
	for (int i = 0; i < displayedItems; ++i) {
		QPointF itemPos = entries[i].pos;
		painter->setBrush(entries[i].rectBrush);
		QRectF rect(itemPos, QSizeF(fontHeight, fontHeight));
		// Decrease box size by legendBoxScale factor
		double delta = fontHeight * (1.0 - legendBoxScale) / 2.0;
		rect = rect.adjusted(delta, delta, -delta, -delta);
		painter->drawRect(rect);
	}

	// Paint labels
	painter->setPen(theme.darkLabelColor); // QPainter uses pen not brush for text!
	painter->setFont(theme.legendFont);
	for (int i = 0; i < displayedItems; ++i) {
		QPointF itemPos = entries[i].pos;
		itemPos.rx() += fontHeight + 2.0 * legendBoxBorderSize;
		QRectF rect(itemPos, QSizeF(entries[i].width, fontHeight));
		painter->drawText(rect, entries[i].name);
	}

	if (!posInitialized) {
		// At first, place in top right corner
		setPos(QPointF(size.width() - width - 10.0, 10.0));
		posInitialized = true;
	} else {
		// Try to keep relative position with what it was before
		setPos(QPointF(size.width() * centerPos.x() - width / 2.0,
			       size.height() * centerPos.y() - height / 2.0));
	}
}

void Legend::setPos(QPointF newPos)
{
	// Round the position to integers or horrible artifacts appear (at least on desktop)
	QPointF posInt(round(newPos.x()), round(newPos.y()));

	// Make sure that the center is inside the drawing area,
	// so that the user can't lose the legend.
	QSizeF size = sceneSize();
	if (size.width() < 1.0 || size.height() < 1.0)
		return;
	double widthHalf = floor(width / 2.0);
	double heightHalf = floor(height / 2.0);
	QPointF sanitizedPos(std::clamp(posInt.x(), -widthHalf, size.width() - widthHalf - 1.0),
			     std::clamp(posInt.y(), -heightHalf, size.height() - heightHalf - 1.0));

	// Set position
	ChartRectItem::setPos(sanitizedPos);

	// Remember relative position of center for next time
	QPointF centerPosAbsolute(sanitizedPos.x() + width / 2.0,
				  sanitizedPos.y() + height / 2.0);
	centerPos = QPointF(centerPosAbsolute.x() / size.width(),
			    centerPosAbsolute.y() / size.height());
}
