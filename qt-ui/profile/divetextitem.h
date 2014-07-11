#ifndef DIVETEXTITEM_H
#define DIVETEXTITEM_H

#include <QObject>
#include <QGraphicsItemGroup>
#include "graphicsview-common.h"
#include <QBrush>

/* A Line Item that has animated-properties. */
class DiveTextItem : public QObject, public QGraphicsItemGroup {
	Q_OBJECT
	Q_PROPERTY(QPointF pos READ pos WRITE setPos)
	Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity)
public:
	DiveTextItem(QGraphicsItem *parent = 0);
	void setText(const QString &text);
	void setAlignment(int alignFlags);
	void setScale(double newscale);
	void setBrush(const QBrush &brush);
	const QString &text();

private:
	void updateText();
	int internalAlignFlags;
	QGraphicsPathItem *textBackgroundItem;
	QGraphicsPathItem *textItem;
	QString internalText;
	color_indice_t colorIndex;
	QBrush brush;
	double scale;
};

#endif // DIVETEXTITEM_H
