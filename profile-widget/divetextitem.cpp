// SPDX-License-Identifier: GPL-2.0
#include "divetextitem.h"
#include "profilewidget2.h"
#include "core/color.h"
#include "core/errorhelper.h"

#include <QBrush>
#include <QDebug>
#include <QApplication>

DiveTextItem::DiveTextItem(double printScale, QGraphicsItem *parent) : QGraphicsItemGroup(parent),
	internalAlignFlags(Qt::AlignHCenter | Qt::AlignVCenter),
	textBackgroundItem(new QGraphicsPathItem(this)),
	textItem(new QGraphicsPathItem(this)),
	printScale(printScale),
	scale(1.0),
	connected(false)
{
	setFlag(ItemIgnoresTransformations);
	textBackgroundItem->setBrush(QBrush(getColor(TEXT_BACKGROUND)));
	textBackgroundItem->setPen(Qt::NoPen);
	textItem->setPen(Qt::NoPen);
}

void DiveTextItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	updateText();
	QGraphicsItemGroup::paint(painter, option, widget);
}

void DiveTextItem::setAlignment(int alignFlags)
{
	if (alignFlags != internalAlignFlags) {
		internalAlignFlags = alignFlags;
	}
}

void DiveTextItem::setBrush(const QBrush &b)
{
	textItem->setBrush(b);
}

void DiveTextItem::setScale(double newscale)
{
	if (scale != newscale) {
		scale = newscale;
	}
}

void DiveTextItem::setText(const QString &t)
{
	if (internalText != t) {
		internalText = t;
		updateText();
	}
}

const QString &DiveTextItem::text()
{
	return internalText;
}

void DiveTextItem::updateText()
{
	double size;
	if (internalText.isEmpty()) {
		return;
	}

	QFont fnt(qApp->font());
	if ((size = fnt.pixelSize()) > 0) {
		// set in pixels - so the scale factor may not make a difference if it's too close to 1
		size *= scale * printScale;
		fnt.setPixelSize(lrint(size));
	} else {
		size = fnt.pointSizeF();
		size *= scale * printScale;
		fnt.setPointSizeF(size);
	}
	QFontMetrics fm(fnt);

	QPainterPath textPath;
	qreal xPos = 0, yPos = 0;

	QRectF rect = fm.boundingRect(internalText);
	yPos = (internalAlignFlags & Qt::AlignTop) ? 0 :
		(internalAlignFlags & Qt::AlignBottom) ? +rect.height() :
		/*(internalAlignFlags & Qt::AlignVCenter  ? */ +rect.height() / 4;

	xPos = (internalAlignFlags & Qt::AlignLeft) ? -rect.width() :
		(internalAlignFlags & Qt::AlignHCenter) ? -rect.width() / 2 :
		/* (internalAlignFlags & Qt::AlignRight) */ 0;

	textPath.addText(xPos, yPos, fnt, internalText);
	QPainterPathStroker stroker;
	stroker.setWidth(3);
	textBackgroundItem->setPath(stroker.createStroke(textPath));
	textItem->setPath(textPath);
}
