#ifndef DIVEEVENTITEM_H
#define DIVEEVENTITEM_H

#include "divepixmapitem.h"

class DiveCartesianAxis;
class DivePlotDataModel;
struct event;

class DiveEventItem : public DivePixmapItem {
	Q_OBJECT
public:
	DiveEventItem(QObject* parent = 0);
	void setEvent(struct event *ev);
	void eventVisibilityChanged(const QString& eventName, bool visible);
	void setVerticalAxis(DiveCartesianAxis *axis);
	void setHorizontalAxis(DiveCartesianAxis *axis);
	void setModel(DivePlotDataModel *model);
private:
	void setupToolTipString();
	void recalculatePos();
	void setupPixmap();
	DiveCartesianAxis *vAxis;
	DiveCartesianAxis *hAxis;
	DivePlotDataModel *dataModel;
	struct event* internalEvent;
};

#endif