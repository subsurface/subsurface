// SPDX-License-Identifier: GPL-2.0
#include "regressionitem.h"
#include "statsaxis.h"
#include "statscolors.h"
#include "statsview.h"
#include "zvalues.h"

#include <cmath>

static const double regressionLineWidth = 2.0;

RegressionItem::RegressionItem(ChartView &view, const StatsTheme &theme, regression_data reg,
			       StatsAxis *xAxis, StatsAxis *yAxis) :
	ChartPixmapItem(view, ChartZValue::ChartFeatures),
	theme(theme),
	xAxis(xAxis), yAxis(yAxis), reg(reg),
	regression(true), confidence(true)
{
}

RegressionItem::~RegressionItem()
{
}

void RegressionItem::setFeatures(bool regressionIn, bool confidenceIn)
{
	if (regressionIn == regression && confidenceIn == confidence)
		return;
	regression = regressionIn;
	confidence = confidenceIn;
	updatePosition();
}

// Note: this calculates the confidence area, even if it isn't shown. Might want to optimize this.
void RegressionItem::updatePosition()
{
	if (!xAxis || !yAxis)
		return;
	auto [minX, maxX] = xAxis->minMax();
	auto [minY, maxY] = yAxis->minMax();
	auto [screenMinX, screenMaxX] = xAxis->minMaxScreen();

	// Draw the confidence interval according to http://www2.stat.duke.edu/~tjl13/s101/slides/unit6lec3H.pdf p.5 with t*=2 for 95% confidence
	QPolygonF poly;
	const int num_samples = 101;
	poly.reserve(num_samples * 2);
	for (int i = 0; i < num_samples; ++i) {
		double x = (maxX - minX) / (num_samples - 1) * static_cast<double>(i) + minX;
		poly << QPointF(xAxis->toScreen(x),
			yAxis->toScreen(reg.a * x + reg.b + 1.960 * sqrt(reg.res2 / (reg.n - 2)  * (1.0 / reg.n + (x - reg.xavg) * (x - reg.xavg) / (reg.n - 1) * (reg.n -2) / reg.sx2))));
	}
	for (int i = num_samples - 1; i >= 0; --i) {
		double x = (maxX - minX) / (num_samples - 1) * static_cast<double>(i) + minX;
		poly << QPointF(xAxis->toScreen(x),
			yAxis->toScreen(reg.a * x + reg.b - 1.960 * sqrt(reg.res2 / (reg.n - 2)  * (1.0 / reg.n + (x - reg.xavg) * (x - reg.xavg) / (reg.n - 1) * (reg.n -2) / reg.sx2))));
	}
	QPolygonF linePolygon;
	linePolygon.reserve(2);
	linePolygon << QPointF(screenMinX, yAxis->toScreen(reg.a * minX + reg.b));
	linePolygon << QPointF(screenMaxX, yAxis->toScreen(reg.a * maxX + reg.b));

	QRectF box(QPointF(screenMinX, yAxis->toScreen(minY)), QPointF(screenMaxX, yAxis->toScreen(maxY)));

	poly = poly.intersected(box);
	linePolygon = linePolygon.intersected(box);
	if (poly.size() < 2 || linePolygon.size() < 2)
		return;

	// Find lowest and highest point on screen. In principle, we need
	// only check half of the polygon, but let's not optimize without reason.
	double screenMinY = std::numeric_limits<double>::max();
	double screenMaxY = std::numeric_limits<double>::lowest();
	for (const QPointF &point: poly) {
		double y = point.y();
		if (y < screenMinY)
			screenMinY = y;
		if (y > screenMaxY)
			screenMaxY = y;
	}
	screenMinY = floor(screenMinY - 1.0);
	screenMaxY = ceil(screenMaxY + 1.0);
	QPointF offset(screenMinX, screenMinY);
	for (QPointF &point: poly)
		point -= offset;
	for (QPointF &point: linePolygon)
		point -= offset;
	ChartPixmapItem::resize(QSizeF(screenMaxX - screenMinX, screenMaxY - screenMinY));

	img->fill(Qt::transparent);
	if (confidence) {
		QColor col(theme.regressionItemColor);
		col.setAlphaF((float)reg.r2);
		painter->setPen(Qt::NoPen);
		painter->setBrush(QBrush(col));
		painter->drawPolygon(poly);
	}

	if (regression) {
		painter->setPen(QPen(theme.regressionItemColor, regressionLineWidth));
		painter->drawLine(QPointF(linePolygon[0]), QPointF(linePolygon[1]));
	}

	ChartPixmapItem::setPos(offset);
}
