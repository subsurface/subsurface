#include "divetextitem.h"
#include "mainwindow.h"
#include "profilewidget2.h"

DiveTextItem::DiveTextItem(QGraphicsItem *parent) : QGraphicsItemGroup(parent),
	internalAlignFlags(Qt::AlignHCenter | Qt::AlignVCenter),
	textBackgroundItem(new QGraphicsPathItem(this)),
	textItem(new QGraphicsPathItem(this)),
	scale(1.0)
{
	setFlag(ItemIgnoresTransformations);
	textBackgroundItem->setBrush(QBrush(getColor(TEXT_BACKGROUND)));
	textBackgroundItem->setPen(Qt::NoPen);
	textItem->setPen(Qt::NoPen);
}

void DiveTextItem::setAlignment(int alignFlags)
{
	if (alignFlags != internalAlignFlags) {
		internalAlignFlags = alignFlags;
		updateText();
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
		updateText();
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
		size *= scale * MainWindow::instance()->graphics()->getFontPrintScale();
		fnt.setPixelSize(size);
	} else {
		size = fnt.pointSizeF();
		size *= scale * MainWindow::instance()->graphics()->getFontPrintScale();
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
