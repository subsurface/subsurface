// SPDX-License-Identifier: GPL-2.0
#ifndef TANKITEM_H
#define TANKITEM_H

#include <QGraphicsItem>
#include <QBrush>
#include "profile-widget/divelineitem.h"
#include "profile-widget/divecartesianaxis.h"
#include "core/dive.h"

class TankItem : public QGraphicsRectItem
{
public:
	explicit TankItem(const DiveCartesianAxis &axis);
	void setData(struct plot_info *plotInfo, struct dive *d);

private:
	void createBar(int startTime, int stopTime, struct gasmix gas);
	void replot();
	const DiveCartesianAxis &hAxis;
	int plotEndTime;
	QBrush air, nitrox, oxygen, trimix;
	QList<QGraphicsRectItem *> rects;
};

#endif // TANKITEM_H
