// SPDX-License-Identifier: GPL-2.0
#include "profile-widget/tankitem.h"
#include "profile-widget/divecartesianaxis.h"
#include "profile-widget/divetextitem.h"
#include "core/dive.h"
#include "core/event.h"
#include "core/profile.h"
#include <QPen>

static const qreal height = 3.0;

TankItem::TankItem(const DiveCartesianAxis &axis) :
	hAxis(axis),
	plotEndTime(-1)
{
	QColor red(PERSIANRED1);
	QColor blue(AIR_BLUE);
	QColor yellow(NITROX_YELLOW);
	QColor green(NITROX_GREEN);
	QLinearGradient nitroxGradient(QPointF(0, 0), QPointF(0, height));
	nitroxGradient.setColorAt(0.0, green);
	nitroxGradient.setColorAt(0.49, green);
	nitroxGradient.setColorAt(0.5, yellow);
	nitroxGradient.setColorAt(1.0, yellow);
	nitrox = nitroxGradient;
	oxygen = green;
	QLinearGradient trimixGradient(QPointF(0, 0), QPointF(0, height));
	trimixGradient.setColorAt(0.0, green);
	trimixGradient.setColorAt(0.49, green);
	trimixGradient.setColorAt(0.5, red);
	trimixGradient.setColorAt(1.0, red);
	trimix = trimixGradient;
	air = blue;
}

void TankItem::createBar(int startTime, int stopTime, struct gasmix gas)
{
	qreal x = hAxis.posAtValue(startTime);
	qreal w = hAxis.posAtValue(stopTime) - hAxis.posAtValue(startTime);

	// pick the right gradient, size, position and text
	QGraphicsRectItem *rect = new QGraphicsRectItem(x, 0, w, height, this);
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
	DiveTextItem *label = new DiveTextItem(rect);
	label->setText(gasname(gas));
	label->setBrush(Qt::black);
	label->setPos(x + 1, 0);
	label->setAlignment(Qt::AlignBottom | Qt::AlignRight);
#ifdef SUBSURFACE_MOBILE
	label->setPos(x + 1, -2.5);
#endif
	label->setZValue(101);
}

void TankItem::setData(const struct plot_info *plotInfo, const struct dive *d)
{
	if (!d)
		return;

	// If there is nothing to plot, quit early.
	if (plotInfo->nr <= 0) {
		plotEndTime = -1;
		return;
	}

	// Find correct end of the dive plot for correct end of the tankbar.
	const struct plot_data *last_entry = &plotInfo->entry[plotInfo->nr - 1];
	plotEndTime = last_entry->sec;

	// We don't have enougth data to calculate things, quit.
	if (plotEndTime < 0)
		return;

	// remove the old rectangles
	qDeleteAll(rects);
	rects.clear();

	// Bail if there are no cylinders
	if (d->cylinders.nr <= 0)
		return;

	// get the information directly from the displayed dive
	// (get_dive_dc() always returns a valid dive computer)
	const struct divecomputer *dc = get_dive_dc_const(d, dc_number);

	// start with the first gasmix and at the start of the dive
	int cyl = explicit_first_cylinder(d, dc);
	struct gasmix gasmix = get_cylinder(d, cyl)->gasmix;
	int startTime = 0;

	// work through all the gas changes and add the rectangle for each gas while it was used
	const struct event *ev = get_next_event(dc->events, "gaschange");
	while (ev && (int)ev->time.seconds < plotEndTime) {
		createBar(startTime, ev->time.seconds, gasmix);
		startTime = ev->time.seconds;
		gasmix = get_gasmix_from_event(d, ev);
		ev = get_next_event(ev->next, "gaschange");
	}
	createBar(startTime, plotEndTime, gasmix);
}
