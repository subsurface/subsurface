#include "diveeventitem.h"
#include "diveplotdatamodel.h"
#include "divecartesianaxis.h"
#include "animationfunctions.h"
#include "libdivecomputer.h"
#include "dive.h"
#include "profile.h"
#include <QDebug>

extern struct ev_select *ev_namelist;
extern int evn_used;

DiveEventItem::DiveEventItem(QObject *parent) : DivePixmapItem(parent),
	vAxis(NULL),
	hAxis(NULL),
	dataModel(NULL),
	internalEvent(NULL)
{
	setFlag(ItemIgnoresTransformations);
}


void DiveEventItem::setHorizontalAxis(DiveCartesianAxis *axis)
{
	hAxis = axis;
	recalculatePos(true);
}

void DiveEventItem::setModel(DivePlotDataModel *model)
{
	dataModel = model;
	recalculatePos(true);
}

void DiveEventItem::setVerticalAxis(DiveCartesianAxis *axis)
{
	vAxis = axis;
	recalculatePos(true);
	connect(vAxis, SIGNAL(sizeChanged()), this, SLOT(recalculatePos()));
}

struct event *DiveEventItem::getEvent()
{
	return internalEvent;
}

void DiveEventItem::setEvent(struct event *ev)
{
	if (!ev)
		return;
	internalEvent = ev;
	setupPixmap();
	setupToolTipString();
	recalculatePos(true);
}

void DiveEventItem::setupPixmap()
{
#define EVENT_PIXMAP(PIX) QPixmap(QString(PIX)).scaled(20, 20, Qt::KeepAspectRatio, Qt::SmoothTransformation)
	if (!internalEvent->name) {
		setPixmap(EVENT_PIXMAP(":warning"));
	} else if ((strcmp(internalEvent->name, "bookmark") == 0)) {
		setPixmap(EVENT_PIXMAP(":flag"));
	} else if (strcmp(internalEvent->name, "heading") == 0) {
		setPixmap(EVENT_PIXMAP(":flag"));
	} else if (internalEvent->type == 123) {
		QPixmap picture;
		picture.load(internalEvent->name);
		setPixmap(picture.scaled(100, 100, Qt::KeepAspectRatio, Qt::SmoothTransformation));
	} else {
		setPixmap(EVENT_PIXMAP(":warning"));
	}
#undef EVENT_PIXMAP
}

void DiveEventItem::setupToolTipString()
{
	// we display the event on screen - so translate
	QString name = tr(internalEvent->name);
	int value = internalEvent->value;
	if (value) {
		if (name == "gaschange") {
			int he = value >> 16;
			int o2 = value & 0xffff;

			name += ": ";
			if (he)
				name += QString("%1/%2").arg(o2).arg(he);
			else if (is_air(o2, he))
				name += tr("air");
			else
				name += QString(tr("EAN%1")).arg(o2);
		} else if (name == "SP change") {
			name += QString(":%1").arg((double)value / 1000);
		} else {
			name += QString(":%1").arg(value);
		}
	} else if (name == "SP change") {
		name += "\n" + tr("Bailing out to OC");
	} else {
		name += internalEvent->flags == SAMPLE_FLAGS_BEGIN ? tr(" begin", "Starts with space!") :
								     internalEvent->flags == SAMPLE_FLAGS_END ? tr(" end", "Starts with space!") : "";
	}
	// qDebug() << name;
	setToolTip(name);
}

void DiveEventItem::eventVisibilityChanged(const QString &eventName, bool visible)
{
}

void DiveEventItem::recalculatePos(bool instant)
{
	bool hidden = false;
	if (!vAxis || !hAxis || !internalEvent || !dataModel)
		return;

	QModelIndexList result = dataModel->match(dataModel->index(0, DivePlotDataModel::TIME), Qt::DisplayRole, internalEvent->time.seconds);
	if (result.isEmpty()) {
		Q_ASSERT("can't find a spot in the dataModel");
		hide();
		return;
	}
	for (int i = 0; i < evn_used; i++) {
		if (!strcmp(internalEvent->name, ev_namelist[i].ev_name) && ev_namelist[i].plot_ev == false)
			hidden = true;
	}
	if (!isVisible() && !hidden)
		show();
	int depth = dataModel->data(dataModel->index(result.first().row(), DivePlotDataModel::DEPTH)).toInt();
	qreal x = hAxis->posAtValue(internalEvent->time.seconds);
	qreal y = vAxis->posAtValue(depth);
	if (!instant)
		Animations::moveTo(this, x, y);
	else
		setPos(x, y);
	if (isVisible() && hidden)
		hide();
}
