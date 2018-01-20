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
	DiveEventItem(QObject *parent = 0);
	virtual ~DiveEventItem();
	void setEvent(struct event *ev, struct gasmix *lastgasmix);
	struct event *getEvent();
	void eventVisibilityChanged(const QString &eventName, bool visible);
	void setVerticalAxis(DiveCartesianAxis *axis);
	void setHorizontalAxis(DiveCartesianAxis *axis);
	void setModel(DivePlotDataModel *model);
	bool shouldBeHidden();
public
slots:
	void recalculatePos(bool instant = false);

private:
	void setupToolTipString(struct gasmix *lastgasmix);
	void setupPixmap(struct gasmix *lastgasmix);
	DiveCartesianAxis *vAxis;
	DiveCartesianAxis *hAxis;
	DivePlotDataModel *dataModel;
	struct event *internalEvent;
};

#endif // DIVEEVENTITEM_H
