// SPDX-License-Identifier: GPL-2.0
#ifndef DIVEPIXMAPITEM_H
#define DIVEPIXMAPITEM_H

#include <QObject>
#include <QGraphicsPixmapItem>

class DivePixmapItem : public QObject, public QGraphicsPixmapItem {
	Q_OBJECT
	Q_PROPERTY(qreal opacity WRITE setOpacity READ opacity)
	Q_PROPERTY(QPointF pos WRITE setPos READ pos)
	Q_PROPERTY(qreal x WRITE setX READ x)
	Q_PROPERTY(qreal y WRITE setY READ y)
public:
	DivePixmapItem(QGraphicsItem *parent = 0);
};

#endif // DIVEPIXMAPITEM_H
