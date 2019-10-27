// SPDX-License-Identifier: GPL-2.0
#include "profile-widget/tankitem.h"
#include "qt-models/diveplotdatamodel.h"
#include "profile-widget/divetextitem.h"
#include "core/profile.h"
#include <QPen>

static const qreal height = 3.0;

TankItem::TankItem(QObject *parent) :
	QObject(parent),
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
	hAxis = nullptr;
}

void TankItem::setData(DivePlotDataModel *model, struct plot_info *plotInfo, struct dive *d)
{
	// If there is nothing to plot, quit early.
	if (plotInfo->nr <= 0) {
		plotEndTime = -1;
		return;
	}

	// Find correct end of the dive plot for correct end of the tankbar.
	struct plot_data *last_entry = &plotInfo->entry[plotInfo->nr - 1];
	plotEndTime = last_entry->sec;

	// Stay informed of changes to the tanks.
	connect(model, SIGNAL(dataChanged(QModelIndex, QModelIndex)), this, SLOT(modelDataChanged(QModelIndex, QModelIndex)), Qt::UniqueConnection);

	modelDataChanged();
}

void TankItem::createBar(int startTime, int stopTime, struct gasmix gas)
{
	qreal x = hAxis->posAtValue(startTime);
	qreal w = hAxis->posAtValue(stopTime) - hAxis->posAtValue(startTime);

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

void TankItem::modelDataChanged(const QModelIndex&, const QModelIndex&)
{
	// We don't have enougth data to calculate things, quit.
	if (plotEndTime < 0)
		return;

	// remove the old rectangles
	qDeleteAll(rects);
	rects.clear();

	// get the information directly from the displayed_dive (the dc always exists)
	struct divecomputer *dc = get_dive_dc(&displayed_dive, dc_number);

	// start with the first gasmix and at the start of the dive
	int cyl = explicit_first_cylinder(&displayed_dive, dc);
	struct gasmix gasmix = displayed_dive.cylinder[cyl].gasmix;
	int startTime = 0;

	// work through all the gas changes and add the rectangle for each gas while it was used
	const struct event *ev = get_next_event(dc->events, "gaschange");
	while (ev && (int)ev->time.seconds < plotEndTime) {
		createBar(startTime, ev->time.seconds, gasmix);
		startTime = ev->time.seconds;
		gasmix = get_gasmix_from_event(&displayed_dive, ev);
		ev = get_next_event(ev->next, "gaschange");
	}
	createBar(startTime, plotEndTime, gasmix);
}

void TankItem::setHorizontalAxis(DiveCartesianAxis *horizontal)
{
	hAxis = horizontal;
	connect(hAxis, SIGNAL(sizeChanged()), this, SLOT(modelDataChanged()));
	modelDataChanged();
}
