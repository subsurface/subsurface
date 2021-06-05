// SPDX-License-Identifier: GPL-2.0
#ifndef DIVEEVENTITEM_H
#define DIVEEVENTITEM_H

#include "divepixmapitem.h"

class DiveCartesianAxis;
class DivePlotDataModel;
struct event;

class DiveEventItem : public DivePixmapItem {
	Q_OBJECT
public:
	DiveEventItem(const struct dive *d, struct event *ev, struct gasmix lastgasmix, QGraphicsItem *parent = 0);
	~DiveEventItem();
	struct event *getEvent();
	void eventVisibilityChanged(const QString &eventName, bool visible);
	void setVerticalAxis(DiveCartesianAxis *axis, int speed);
	void setHorizontalAxis(DiveCartesianAxis *axis);
	void setModel(DivePlotDataModel *model);
	bool shouldBeHidden();
public
slots:
	void recalculatePos(int animationSpeed);

private:
	void setupToolTipString(struct gasmix lastgasmix);
	void setupPixmap(struct gasmix lastgasmix);
	int depthAtTime(int time);
	DiveCartesianAxis *vAxis;
	DiveCartesianAxis *hAxis;
	DivePlotDataModel *dataModel;
	struct event *internalEvent;
	const struct dive *dive;
};

#endif // DIVEEVENTITEM_H
