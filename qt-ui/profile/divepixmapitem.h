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
	DivePixmapItem(QObject *parent = 0);
};

class DivePictureItem : public DivePixmapItem {
	Q_OBJECT
	Q_PROPERTY(qreal scale WRITE setScale READ scale)
public:
	DivePictureItem(int row, QObject *parent = 0);
	void setPixmap(const QPixmap& pix);
protected:
	void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
	void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);
	void mousePressEvent(QGraphicsSceneMouseEvent *event);
private:
	int rowOnModel;
};

#endif // DIVEPIXMAPITEM_H
