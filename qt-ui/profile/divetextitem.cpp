#include "divetextitem.h"
#include <QPropertyAnimation>

DiveTextItem::DiveTextItem(QGraphicsItem* parent): QGraphicsSimpleTextItem(parent)
{
	setFlag(ItemIgnoresTransformations);
}

void DiveTextItem::setAlignment(int alignFlags)
{
	internalAlignFlags = alignFlags;
	update();
}

void DiveTextItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	/* This block of code corrects paints the Text on the
	 * alignment choosed, but it created artifacts on the screen,
	 * so I'm leaving this disabled for the time being.
	 */
// 	QRectF rect = boundingRect();
// 	if (internalAlignFlags & Qt::AlignTop)
// 		painter->translate(0, -rect.height());
// // 	else if (internalAlignFlags & Qt::AlignBottom)
// // 		painter->translate(0, rect.height()); this is the default, uneeded.
// 	else if (internalAlignFlags & Qt::AlignVCenter)
// 		painter->translate(0, -rect.height() / 2);
//
// // 	if (internalAlignFlags & Qt::AlignLeft )
// // 		painter->translate(); // This is the default, uneeded.
// 	if(internalAlignFlags & Qt::AlignHCenter)
// 		painter->translate(-rect.width()/2, 0);
// 	else if(internalAlignFlags & Qt::AlignRight)
// 		painter->translate(-rect.width(), 0);

	QGraphicsSimpleTextItem::paint(painter, option, widget);
}

void DiveTextItem::animatedHide()
{
	QPropertyAnimation *animation = new QPropertyAnimation(this, "opacity");
	connect(animation, SIGNAL(finished()), SLOT(deleteLater()));
	animation->setStartValue(1);
	animation->setEndValue(0);
	animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void DiveTextItem::animateMoveTo(qreal x, qreal y)
{
	QPropertyAnimation *animation = new QPropertyAnimation(this, "pos");
	animation->setStartValue(property("pos").toPointF());
	animation->setEndValue(QPointF(x, y));
	animation->start(QAbstractAnimation::DeleteWhenStopped);
}