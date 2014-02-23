#include "diveeventitem.h"
#include "diveplotdatamodel.h"
#include "divecartesianaxis.h"
#include "animationfunctions.h"
#include "dive.h"
#include <QDebug>

DiveEventItem::DiveEventItem(QObject* parent): DivePixmapItem(parent),
	vAxis(NULL), hAxis(NULL), dataModel(NULL), internalEvent(NULL)
{
	setFlag(ItemIgnoresTransformations);
}


void DiveEventItem::setHorizontalAxis(DiveCartesianAxis* axis)
{
	hAxis = axis;
	recalculatePos(true);
}

void DiveEventItem::setModel(DivePlotDataModel* model)
{
	dataModel = model;
	recalculatePos(true);
}

void DiveEventItem::setVerticalAxis(DiveCartesianAxis* axis)
{
	vAxis = axis;
	recalculatePos(true);
	connect(vAxis, SIGNAL(sizeChanged()), this, SLOT(recalculatePos()));
}

void DiveEventItem::setEvent(struct event* ev)
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
#define EVENT_PIXMAP( PIX ) QPixmap(QString(PIX)).scaled(20, 20, Qt::KeepAspectRatio, Qt::SmoothTransformation)
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
	//TODO Fix this. :)
#if 0
	This needs to be redone, but right now the events are being plotted and I liked pretty much the code.

	struct dive *dive = getDiveById(diveId);
	Q_ASSERT(dive != NULL);
	EventItem *item = new EventItem(ev, 0, isGrayscale);
	item->setPos(x, y);
	scene()->addItem(item);

	/* we display the event on screen - so translate (with the correct context for events) */
	QString name = gettextFromC::instance()->tr(ev->name);
	if (ev->value) {
		if (ev->name && strcmp(ev->name, "gaschange") == 0) {
			int he = get_he(&dive->cylinder[entry->cylinderindex].gasmix);
			int o2 = get_o2(&dive->cylinder[entry->cylinderindex].gasmix);

			name += ": ";
			if (he)
				name += QString("%1/%2").arg((o2 + 5) / 10).arg((he + 5) / 10);
			else if (is_air(o2, he))
				name += tr("air");
			else
				name += QString(tr("EAN%1")).arg((o2 + 5) / 10);

		} else if (ev->name && !strcmp(ev->name, "SP change")) {
			name += QString(":%1").arg((double) ev->value / 1000);
		} else {
			name += QString(":%1").arg(ev->value);
		}
	} else if (ev->name && name == "SP change") {
		name += "\n" + tr("Bailing out to OC");
	} else {
		name += ev->flags == SAMPLE_FLAGS_BEGIN ? tr(" begin", "Starts with space!") :
				ev->flags == SAMPLE_FLAGS_END ? tr(" end", "Starts with space!") : "";
	}

	//item->setToolTipController(toolTip);
	//item->addToolTip(name);
	item->setToolTip(name);
#endif
}

void DiveEventItem::eventVisibilityChanged(const QString& eventName, bool visible)
{
}

void DiveEventItem::recalculatePos(bool instant)
{
	if (!vAxis || !hAxis || !internalEvent || !dataModel)
		return;

	QModelIndexList result = dataModel->match(dataModel->index(0,DivePlotDataModel::TIME), Qt::DisplayRole, internalEvent->time.seconds );
	if (result.isEmpty()) {
		Q_ASSERT("can't find a spot in the dataModel");
		hide();
		return;
	}
	if (!isVisible())
		show();

	int depth = dataModel->data(dataModel->index(result.first().row(), DivePlotDataModel::DEPTH)).toInt();
	qreal x = hAxis->posAtValue(internalEvent->time.seconds);
	qreal y = vAxis->posAtValue(depth);
	if (!instant)
		Animations::moveTo(this, x, y, 500);
	else
		setPos(x,y);
}
