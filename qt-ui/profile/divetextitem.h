#ifndef DIVETEXTITEM_H
#define DIVETEXTITEM_H

#include <QObject>
#include <QGraphicsSimpleTextItem>

/* A Line Item that has animated-properties. */
class DiveTextItem :public QObject, public QGraphicsSimpleTextItem{
	Q_OBJECT
	Q_PROPERTY(QPointF pos READ pos WRITE setPos)
	Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity)
public:
	DiveTextItem(QGraphicsItem* parent = 0);
	virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);
	void setAlignment(int alignFlags);
	void animatedHide();
	void animateMoveTo(qreal x, qreal y);
private:
	int internalAlignFlags;
};

#endif
