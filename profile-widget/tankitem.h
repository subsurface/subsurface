// SPDX-License-Identifier: GPL-2.0
#ifndef TANKITEM_H
#define TANKITEM_H

#include "profile-widget/divelineitem.h"
#include "core/gas.h"
#include <QGraphicsRectItem>
#include <QBrush>

struct dive;
class DiveCartesianAxis;

class TankItem : public QGraphicsRectItem
{
public:
	explicit TankItem(const DiveCartesianAxis &axis);
	void setData(const struct plot_info *plotInfo, const struct dive *d);

private:
	void createBar(int startTime, int stopTime, struct gasmix gas);
	const DiveCartesianAxis &hAxis;
	int plotEndTime;
	QBrush air, nitrox, oxygen, trimix;
	QList<QGraphicsRectItem *> rects;
};

#endif // TANKITEM_H
