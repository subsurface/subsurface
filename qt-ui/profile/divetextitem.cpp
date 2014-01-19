#include "divetextitem.h"
#include "animationfunctions.h"

#include <QPropertyAnimation>
#include <QApplication>
#include <QFont>
#include <QFontMetrics>
#include <QBrush>
#include <QPen>

DiveTextItem::DiveTextItem(QGraphicsItem* parent): QGraphicsItemGroup(parent),
	textBackgroundItem(NULL),
	textItem(NULL),
	internalAlignFlags(0)
{
	setFlag(ItemIgnoresTransformations);
}

void DiveTextItem::setAlignment(int alignFlags)
{
	internalAlignFlags = alignFlags;
	updateText();
}

void DiveTextItem::setBrush(const QBrush& b)
{
	brush = b;
	updateText();
}

void DiveTextItem::setText(const QString& t)
{
	text = t;
	updateText();
}

void DiveTextItem::updateText()
{
	if(!internalAlignFlags || text.isEmpty())
		return;

	delete textItem;
	delete textBackgroundItem;

	QFont fnt(qApp->font());
	QFontMetrics fm(fnt);

	QPainterPath textPath;
	qreal xPos = 0, yPos = 0;

	QRectF rect = fm.boundingRect(text);
	yPos = (internalAlignFlags & Qt::AlignTop) ? -rect.height() :
			(internalAlignFlags & Qt::AlignBottom) ? 0 :
	/*(internalAlignFlags & Qt::AlignVCenter  ? */ -rect.height() / 2;

	yPos = (internalAlignFlags & Qt::AlignLeft ) ? 0 :
		(internalAlignFlags & Qt::AlignHCenter) ?  -rect.width()/2 :
	 /* (internalAlignFlags & Qt::AlignRight) */ -rect.width();

	textPath.addText( xPos, yPos, fnt, text);
	QPainterPathStroker stroker;
	stroker.setWidth(3);
	textBackgroundItem = new QGraphicsPathItem(stroker.createStroke(textPath), this);
	textBackgroundItem->setBrush(QBrush(getColor(TEXT_BACKGROUND)));
	textBackgroundItem->setPen(Qt::NoPen);

	textItem = new QGraphicsPathItem(textPath, this);
	textItem->setBrush(brush);
	textItem->setPen(Qt::NoPen);
}

void DiveTextItem::animatedHide()
{
	Animations::hide(this);
}

void DiveTextItem::animateMoveTo(qreal x, qreal y)
{
	Animations::moveTo(this, x, y);
}
