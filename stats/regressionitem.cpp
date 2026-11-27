// SPDX-License-Identifier: GPL-2.0
#include "regressionitem.h"
#include "statsaxis.h"
#include "statscolors.h"
#include "statsview.h"
#include "zvalues.h"

#include <cmath>
#include <limits>

static const double regressionLineWidth = 2.0;

RegressionItem::RegressionItem(StatsView &view, regression_data reg,
						   StatsAxis *xAxis, StatsAxis *yAxis) :
	ChartPixmapItem(view, ChartZValue::ChartFeatures),
	theme(view.getCurrentTheme()),
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

// AI-generated (GPT-6 Sol): keep the line independent of confidence geometry.
void RegressionItem::updatePosition()
{
	auto clearImage = [this] {
		if (img) {
			img->fill(Qt::transparent);
			setTextureDirty();
		}
	};
	if (!xAxis || !yAxis)
		return clearImage();
	auto [minX, maxX] = xAxis->minMax();
	auto [minY, maxY] = yAxis->minMax();
	auto [screenMinX, screenMaxX] = xAxis->minMaxScreen();
	double top = yAxis->toScreen(maxY);
	double bottom = yAxis->toScreen(minY);
	if (!std::isfinite(minX) || !std::isfinite(maxX) || !std::isfinite(reg.a) ||
	    !std::isfinite(reg.b) || !std::isfinite(screenMinX) || !std::isfinite(screenMaxX) ||
	    !std::isfinite(top) || !std::isfinite(bottom) || screenMaxX <= screenMinX || bottom <= top)
		return clearImage();
	QRectF box(QPointF(screenMinX, top), QPointF(screenMaxX, bottom));

	// Draw the confidence interval according to http://www2.stat.duke.edu/~tjl13/s101/slides/unit6lec3H.pdf p.5 with t*=2 for 95% confidence
	QPolygonF poly;
	if (confidence && reg.n > 2 && reg.sx2 > 0.0 && std::isfinite(reg.res2) &&
	    reg.res2 >= 0.0 && std::isfinite(reg.sx2) && std::isfinite(reg.xavg) &&
	    std::isfinite(reg.r2)) {
		const int num_samples = 101;
		poly.reserve(num_samples * 2);
		bool valid = true;
		for (int side : { 1, -1 }) {
			for (int index = 0; index < num_samples; ++index) {
				int i = side == 1 ? index : num_samples - 1 - index;
				double x = (maxX - minX) / (num_samples - 1) * static_cast<double>(i) + minX;
				double dx = x - reg.xavg;
				double variance = reg.res2 / (reg.n - 2) *
					(1.0 / reg.n + dx * dx / (reg.n - 1) * (reg.n - 2) / reg.sx2);
				QPointF point(xAxis->toScreen(x),
					      yAxis->toScreen(reg.a * x + reg.b + side * 1.960 * sqrt(variance)));
				if (!std::isfinite(point.x()) || !std::isfinite(point.y())) {
					valid = false;
					break;
				}
				poly << point;
			}
			if (!valid)
				break;
		}
		if (valid)
			poly = poly.intersected(box);
		else
			poly.clear();
	}

	// Clip the line as a segment, not as a polygon (a two-point polygon has no area).
	QPointF from(screenMinX, yAxis->toScreen(reg.a * minX + reg.b));
	QPointF to(screenMaxX, yAxis->toScreen(reg.a * maxX + reg.b));
	bool hasBandLine = confidence && reg.n > 2 && reg.res2 == 0.0 &&
			   reg.sx2 > 0.0 && std::isfinite(reg.sx2) && std::isfinite(reg.r2);
	bool hasLine = (regression || hasBandLine) && std::isfinite(from.y()) && std::isfinite(to.y());
	if (hasLine && from.y() != to.y()) {
		if (from.y() < top || from.y() > bottom) {
			double t = ((from.y() < top ? top : bottom) - from.y()) / (to.y() - from.y());
			from += t * (to - from);
		}
		if (to.y() < top || to.y() > bottom) {
			double t = ((to.y() < top ? top : bottom) - to.y()) / (from.y() - to.y());
			to += t * (from - to);
		}
	}
	hasLine = hasLine && box.contains(from) && box.contains(to) && from != to;
	if (poly.size() < 3)
		poly.clear();

	// Include both independently clipped features in the canvas bounds.
	double screenMinY = std::numeric_limits<double>::max();
	double screenMaxY = std::numeric_limits<double>::lowest();
	if (hasLine) {
		screenMinY = std::min(from.y(), to.y());
		screenMaxY = std::max(from.y(), to.y());
	}
	for (const QPointF &point: poly) {
		screenMinY = std::min(screenMinY, point.y());
		screenMaxY = std::max(screenMaxY, point.y());
	}
	if (!hasLine && poly.empty()) {
		// An existing texture must be cleared when the last drawable feature disappears.
		return clearImage();
	}
	screenMinY = floor(screenMinY - 1.0);
	screenMaxY = ceil(screenMaxY + 1.0);
	double canvasWidth = screenMaxX - screenMinX;
	double canvasHeight = screenMaxY - screenMinY;
	if (!std::isfinite(screenMinY) || !std::isfinite(screenMaxY) ||
	    canvasWidth > 16384 || canvasHeight > 16384 ||
	    canvasWidth * canvasHeight > 16000000)
		return clearImage();
	QPointF offset(screenMinX, screenMinY);
	for (QPointF &point: poly)
		point -= offset;
	from -= offset;
	to -= offset;
	ChartPixmapItem::resize(QSizeF(canvasWidth, canvasHeight));

	img->fill(Qt::transparent);
	if (!poly.empty()) {
		QColor col(theme.regressionItemColor);
		col.setAlphaF(static_cast<float>(qBound(0.0, reg.r2, 1.0)));
		painter->setPen(Qt::NoPen);
		painter->setBrush(QBrush(col));
		painter->drawPolygon(poly);
	}

	if (hasLine) {
		QColor color(theme.regressionItemColor);
		if (!regression)
			color.setAlphaF(static_cast<float>(qBound(0.0, reg.r2, 1.0)));
		painter->setPen(QPen(color, regressionLineWidth));
		painter->drawLine(from, to);
	}

	ChartPixmapItem::setPos(offset);
}
