#include "divepixmapitem.h"
#include "animationfunctions.h"
#include <divepicturewidget.h>

#include <QPen>
#include <QBrush>
#include <QGraphicsDropShadowEffect>
#include <QDesktopServices>
#include <QUrl>

DivePixmapItem::DivePixmapItem(QObject *parent) : QObject(parent), QGraphicsPixmapItem()
{
}

DivePictureItem::DivePictureItem(int row, QObject *parent): DivePixmapItem(parent)
{
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
	setAcceptsHoverEvents(true);
#else
	setAcceptHoverEvents(true);
#endif
	rowOnModel = row;
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

	qreal angle = qrand() % 5;
	if (rand() % 2)
		angle *= -1;

	setRotation(angle);
}

void DivePictureItem::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
	Animations::scaleTo(this, 1.0);
	this->setZValue(5);
}

void DivePictureItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
	Animations::scaleTo(this, 0.2);
	this->setZValue(0);
}

void DivePictureItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	QString filePath = DivePictureModel::instance()->index(rowOnModel,0).data(Qt::ToolTipRole).toString();
	QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));
}
