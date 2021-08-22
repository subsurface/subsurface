// SPDX-License-Identifier: GPL-2.0
#include "divetextitem.h"
#include "profilewidget2.h"
#include "core/color.h"
#include "core/errorhelper.h"

#include <QBrush>
#include <QApplication>

static const double outlineSize = 3.0;

DiveTextItem::DiveTextItem(double dpr, double scale, int alignFlags, QGraphicsItem *parent) : QGraphicsPixmapItem(parent),
	internalAlignFlags(alignFlags),
	dpr(dpr),
	scale(scale)
{
	setFlag(ItemIgnoresTransformations);
}

void DiveTextItem::set(const QString &t, const QBrush &b)
{
	internalText = t;
	if (internalText.isEmpty()) {
		setPixmap(QPixmap());
		return;
	}

	QFont fnt = getFont(dpr, scale);

	QPainterPath textPath;
	textPath.addText(0.0, 0.0, fnt, internalText);
	QPainterPathStroker stroker;
	stroker.setWidth(outlineSize * dpr);
	QPainterPath outlinePath = stroker.createStroke(textPath);

	QRectF outlineRect = outlinePath.boundingRect();
	textPath.translate(-outlineRect.topLeft());
	outlinePath.translate(-outlineRect.topLeft());

	QPixmap pixmap(outlineRect.size().toSize());
	pixmap.fill(Qt::transparent);
	{
		QPainter painter(&pixmap);
		painter.setRenderHints(QPainter::Antialiasing);
		painter.setBrush(QBrush(getColor(TEXT_BACKGROUND)));
		painter.setPen(Qt::NoPen);
		painter.drawPath(outlinePath);
		painter.setBrush(b);
		painter.setPen(Qt::NoPen);
		painter.drawPath(textPath);
	}
	setPixmap(pixmap);

	double yOffset = (internalAlignFlags & Qt::AlignTop) ? 0.0 :
		(internalAlignFlags & Qt::AlignBottom) ? -outlineRect.height() :
		/*(internalAlignFlags & Qt::AlignVCenter  ? */ -outlineRect.height() / 2.0;

	double xOffset = (internalAlignFlags & Qt::AlignLeft) ? -outlineRect.width() :
		(internalAlignFlags & Qt::AlignHCenter) ? -outlineRect.width() / 2.0 :
		/* (internalAlignFlags & Qt::AlignRight) */ 0.0;
	setOffset(xOffset, yOffset);
}

const QString &DiveTextItem::text()
{
	return internalText;
}

QFont DiveTextItem::getFont(double dpr, double scale)
{
	QFont fnt(qApp->font());
	double size = fnt.pixelSize();
	if (size > 0) {
		// set in pixels - so the scale factor may not make a difference if it's too close to 1
		size *= scale * dpr;
		fnt.setPixelSize(lrint(size));
	} else {
		size = fnt.pointSizeF();
		size *= scale * dpr;
		fnt.setPointSizeF(size);
	}
	return fnt;
}

double DiveTextItem::fontHeight(double dpr, double scale)
{
	QFont fnt = getFont(dpr, scale);
	QFontMetrics fm(fnt);
	return (double)fm.height();
}

double DiveTextItem::height() const
{
	return fontHeight(dpr, scale) + outlineSize * dpr;
}
