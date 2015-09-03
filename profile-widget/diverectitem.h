#ifndef DIVERECTITEM_H
#define DIVERECTITEM_H

#include <QObject>
#include <QGraphicsRectItem>

class DiveRectItem : public QObject, public QGraphicsRectItem {
	Q_OBJECT
	Q_PROPERTY(QRectF rect WRITE setRect READ rect)
	Q_PROPERTY(QPointF pos WRITE setPos READ pos)
	Q_PROPERTY(qreal x WRITE setX READ x)
	Q_PROPERTY(qreal y WRITE setY READ y)
public:
	DiveRectItem(QObject *parent = 0, QGraphicsItem *parentItem = 0);
};

#endif // DIVERECTITEM_H
