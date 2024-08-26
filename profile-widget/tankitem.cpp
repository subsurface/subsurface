// SPDX-License-Identifier: GPL-2.0
#include "profile-widget/tankitem.h"
#include "profile-widget/divecartesianaxis.h"
#include "profile-widget/divetextitem.h"
#include "core/dive.h"
#include "core/event.h"
#include "core/profile.h"
#include <QPen>
#include <QFontMetrics>

static const double border = 1.0;

TankItem::TankItem(const DiveCartesianAxis &axis, double dpr) :
	hAxis(axis),
	dpr(dpr)
{
	QColor red(PERSIANRED1);
	QColor blue(AIR_BLUE);
	QColor yellow(NITROX_YELLOW);
	QColor green(NITROX_GREEN);
	QLinearGradient nitroxGradient(QPointF(0, 0), QPointF(0, height()));
	nitroxGradient.setColorAt(0.0, green);
	nitroxGradient.setColorAt(0.49, green);
	nitroxGradient.setColorAt(0.5, yellow);
	nitroxGradient.setColorAt(1.0, yellow);
	nitrox = nitroxGradient;
	oxygen = green;
	QLinearGradient trimixGradient(QPointF(0, 0), QPointF(0, height()));
	trimixGradient.setColorAt(0.0, green);
	trimixGradient.setColorAt(0.49, green);
	trimixGradient.setColorAt(0.5, red);
	trimixGradient.setColorAt(1.0, red);
	trimix = trimixGradient;
	air = blue;
}

double TankItem::height() const
{
	return DiveTextItem::fontHeight(dpr, 1.0) + 2.0 * border * dpr;
}

void TankItem::createBar(int startTime, int stopTime, struct gasmix gas)
{
	qreal x = hAxis.posAtValue(startTime);
	qreal w = hAxis.posAtValue(stopTime) - hAxis.posAtValue(startTime);

	// pick the right gradient, size, position and text
	QGraphicsRectItem *rect = new QGraphicsRectItem(x, 0, w, height(), this);
	if (gasmix_is_air(gas))
		rect->setBrush(air);
	else if (gas.he.permille)
		rect->setBrush(trimix);
	else if (gas.o2.permille == 1000)
		rect->setBrush(oxygen);
	else
		rect->setBrush(nitrox);
	rect->setPen(QPen(QBrush(), 0.0)); // get rid of the thick line around the rectangle
	rects.push_back(rect);
	DiveTextItem *label = new DiveTextItem(dpr, 1.0, Qt::AlignVCenter | Qt::AlignRight, rect);
	label->set(QString::fromStdString(gas.name()), Qt::black);
	label->setPos(x + 2.0 * dpr, height() / 2.0);
	label->setZValue(101);
}

void TankItem::setData(const struct dive *d, const struct divecomputer *dc, int plotStartTime, int plotEndTime)
{
	// remove the old rectangles
	qDeleteAll(rects);
	rects.clear();

	if (!d || !dc)
		return;

	// We don't have enougth data to calculate things, quit.
	if (plotEndTime < 0 || plotEndTime <= plotStartTime)
		return;

	// Bail if there are no cylinders
	if (d->cylinders.empty())
		return;

	// start with the first gasmix and at the start of the plotted range
	// and work through all the gas changes and add the rectangle for each gas while it was used
	gasmix_loop loop(*d, *dc);
	struct gasmix next_gasmix = loop.at(plotStartTime).first;
	int next_startTime = plotStartTime;
	while (loop.has_next()) {
		auto [gasmix, time] = loop.next();
		createBar(next_startTime, time, next_gasmix);
		next_startTime = time;
		next_gasmix = gasmix;
	}

	createBar(next_startTime, plotEndTime, next_gasmix);
}
