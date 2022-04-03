// SPDX-License-Identifier: GPL-2.0
#ifndef TANKITEM_H
#define TANKITEM_H

#include "profile-widget/divelineitem.h"
#include "core/gas.h"
#include <QGraphicsRectItem>
#include <QBrush>

struct dive;
struct divecomputer;
class DiveCartesianAxis;

class TankItem : public QGraphicsRectItem
{
public:
	explicit TankItem(const DiveCartesianAxis &axis, double dpr);
	void setData(const struct dive *d, const struct divecomputer *dc, int plotStartTime, int plotEndTime);
	double height() const;

private:
	void createBar(int startTime, int stopTime, struct gasmix gas);
	const DiveCartesianAxis &hAxis;
	double dpr;
	QBrush air, nitrox, oxygen, trimix;
	QList<QGraphicsRectItem *> rects;
};

#endif // TANKITEM_H
