#include "divetextitem.h"
#include "animationfunctions.h"

#include <QPropertyAnimation>
#include <QApplication>
#include <QFont>
#include <QFontMetrics>
#include <QBrush>
#include <QPen>
#include <QDebug>

DiveTextItem::DiveTextItem(QGraphicsItem* parent): QGraphicsItemGroup(parent),
	textBackgroundItem(NULL),
	textItem(NULL),
	internalAlignFlags(Qt::AlignHCenter | Qt::AlignVCenter)
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
	internalText = t;
	updateText();
}

const QString& DiveTextItem::text()
{
	return internalText;
}

void DiveTextItem::updateText()
{
	if(internalText.isEmpty())
		return;

	delete textItem;
	delete textBackgroundItem;

	QFont fnt(qApp->font());
	QFontMetrics fm(fnt);

	QPainterPath textPath;
	qreal xPos = 0, yPos = 0;

	QRectF rect = fm.boundingRect(internalText);
	yPos = (internalAlignFlags & Qt::AlignTop) ? -rect.height() :
			(internalAlignFlags & Qt::AlignBottom) ? +rect.height() :
	/*(internalAlignFlags & Qt::AlignVCenter  ? */ +rect.height() / 4;

	xPos = (internalAlignFlags & Qt::AlignLeft ) ? +rect.width() :
		(internalAlignFlags & Qt::AlignHCenter) ?  -rect.width()/2 :
	 /* (internalAlignFlags & Qt::AlignRight) */ -rect.width();

	textPath.addText( xPos, yPos, fnt, internalText);
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
