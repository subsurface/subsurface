#include "divepixmapitem.h"
#include "animationfunctions.h"
DivePixmapItem::DivePixmapItem(QObject *parent) : QObject(parent), QGraphicsPixmapItem()
{
}

DivePictureItem::DivePictureItem(QObject *parent): DivePixmapItem(parent)
{
	setAcceptsHoverEvents(true);
	setScale(0.2);
}

void DivePictureItem::setPixmap(const QPixmap &pix)
{
	DivePixmapItem::setPixmap(pix);
	setTransformOriginPoint(boundingRect().width()/2, boundingRect().height()/2);
}

void DivePictureItem::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
	Animations::scaleTo(this, 1.0);
}

void DivePictureItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
	Animations::scaleTo(this, 0.2);
}
