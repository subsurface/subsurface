// SPDX-License-Identifier: GPL-2.0
#ifndef DIVEEVENTITEM_H
#define DIVEEVENTITEM_H

#include "divepixmapitem.h"

class DiveCartesianAxis;
class DivePixmapCache;
class DivePixmaps;
struct event;

class DiveEventItem : public DivePixmapItem {
	Q_OBJECT
public:
	DiveEventItem(const struct dive *d, struct event *ev, struct gasmix lastgasmix,
		      DivePlotDataModel *model, DiveCartesianAxis *hAxis, DiveCartesianAxis *vAxis,
		      int speed, const DivePixmaps &pixmaps, QGraphicsItem *parent = nullptr);
	~DiveEventItem();
	const struct event *getEvent() const;
	struct event *getEventMutable();
	void eventVisibilityChanged(const QString &eventName, bool visible);
	void setVerticalAxis(DiveCartesianAxis *axis, int speed);
	void setHorizontalAxis(DiveCartesianAxis *axis);
	void setModel(DivePlotDataModel *model);
	bool shouldBeHidden();
	static bool isInteresting(const struct dive *d, const struct divecomputer *dc,
				  const struct event *ev, const DivePlotDataModel &model);
public
slots:
	void recalculatePos(int animationSpeed);

private:
	void setupToolTipString(struct gasmix lastgasmix);
	void setupPixmap(struct gasmix lastgasmix, const DivePixmaps &pixmaps);
	DiveCartesianAxis *vAxis;
	DiveCartesianAxis *hAxis;
	DivePlotDataModel *dataModel;
	struct event *ev;
	const struct dive *dive;
};

#endif // DIVEEVENTITEM_H
