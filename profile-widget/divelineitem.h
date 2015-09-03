#ifndef DIVELINEITEM_H
#define DIVELINEITEM_H

#include <QObject>
#include <QGraphicsLineItem>

class DiveLineItem : public QObject, public QGraphicsLineItem {
	Q_OBJECT
	Q_PROPERTY(QPointF pos READ pos WRITE setPos)
	Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity)
public:
	DiveLineItem(QGraphicsItem *parent = 0);
};

#endif // DIVELINEITEM_H
