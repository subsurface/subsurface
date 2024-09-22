// SPDX-License-Identifier: GPL-2.0
#ifndef DIVEEVENTITEM_H
#define DIVEEVENTITEM_H

#include "divepixmapitem.h"
#include "core/event.h"

class DiveCartesianAxis;
struct DivePixmaps;
struct event;
struct plot_info;

class DiveEventItem : public DivePixmapItem {
	Q_OBJECT
public:
	DiveEventItem(const struct dive *d, const struct divecomputer *dc, int idx, const struct event &ev, const struct gasmix lastgasmix, divemode_t lastdivemode,
		      const struct plot_info &pi, DiveCartesianAxis *hAxis, DiveCartesianAxis *vAxis,
		      int speed, const DivePixmaps &pixmaps, QGraphicsItem *parent = nullptr);
	~DiveEventItem();
	void eventVisibilityChanged(const QString &eventName, bool visible);
	void setVerticalAxis(DiveCartesianAxis *axis, int speed);
	void setHorizontalAxis(DiveCartesianAxis *axis);
	static bool isInteresting(const struct dive *d, const struct divecomputer *dc,
				  const struct event &ev, const struct plot_info &pi,
				  int firstSecond, int lastSecond);

private:
	void setupToolTipString(struct gasmix lastgasmix, divemode_t lastdivemode, const struct divecomputer &dc);
	void setupPixmap(struct gasmix lastgasmix, divemode_t lastdivemode, const struct divecomputer &dc, const DivePixmaps &pixmaps);
	void recalculatePos();
	DiveCartesianAxis *vAxis;
	DiveCartesianAxis *hAxis;
public:
	int idx;
	struct event ev;
	const struct dive *dive;
	int depth;
};

#endif // DIVEEVENTITEM_H
