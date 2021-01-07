#include "roundrectitem.h"
#include <QPainter>
#include <QStyleOptionGraphicsItem>

RoundRectItem::RoundRectItem(double radius, QGraphicsItem *parent) : QGraphicsRectItem(parent),
	radius(radius)
{
}

RoundRectItem::RoundRectItem(double radius) : RoundRectItem(radius, nullptr)
{
}

void RoundRectItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *)
{
	painter->save();
	painter->setClipRect(option->rect);
	painter->setPen(pen());
	painter->setBrush(brush());
	painter->drawRoundedRect(rect(), radius, radius, Qt::AbsoluteSize);
	painter->restore();
}
