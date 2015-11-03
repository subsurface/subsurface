#ifndef DIVETEXTITEM_H
#define DIVETEXTITEM_H

#include <QObject>
#include <QGraphicsItemGroup>

class QBrush;

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
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

private
slots:
	void fontPrintScaleUpdate(double scale);

private:
	void updateText();
	int internalAlignFlags;
	QGraphicsPathItem *textBackgroundItem;
	QGraphicsPathItem *textItem;
	QString internalText;
	double printScale;
	double scale;
	bool connected;
};

#endif // DIVETEXTITEM_H
