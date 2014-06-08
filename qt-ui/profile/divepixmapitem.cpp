#include "divepixmapitem.h"
#include "animationfunctions.h"

#include <QPen>
#include <QBrush>
#include <QGraphicsDropShadowEffect>

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
	QRectF r = boundingRect();
	QGraphicsRectItem *rect = new QGraphicsRectItem(0 - 10, 0 -10, r.width() + 20, r.height() + 20, this);
	rect->setPen(Qt::NoPen);
	rect->setBrush(QColor(Qt::white));
	rect->setFlag(ItemStacksBehindParent);
	rect->setZValue(-1);

	QGraphicsRectItem *shadow = new QGraphicsRectItem(rect->boundingRect(), this);
	shadow->setPos(5,5);
	shadow->setPen(Qt::NoPen);
	shadow->setBrush(QColor(Qt::lightGray));
	shadow->setFlag(ItemStacksBehindParent);
	shadow->setZValue(-2);

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
