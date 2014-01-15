#ifndef DIVECARTESIANAXIS_H
#define DIVECARTESIANAXIS_H

#include <QObject>
#include <QGraphicsLineItem>
class DiveTextItem;
class DiveLineItem;

class DiveCartesianAxis : public QObject, public QGraphicsLineItem{
	Q_OBJECT
	Q_PROPERTY(QLineF line WRITE setLine READ line)
	Q_PROPERTY(QPointF pos WRITE setPos READ pos)
	Q_PROPERTY(qreal x WRITE setX READ x)
	Q_PROPERTY(qreal y WRITE setY READ y)
public:
	DiveCartesianAxis();
	virtual ~DiveCartesianAxis();
	void setMinimum(double minimum);
	void setMaximum(double maximum);
	void setTickInterval(double interval);
	void setOrientation(Qt::Orientation orientation);
	void setTickSize(qreal size);
	void updateTicks();
	double minimum() const;
	double maximum() const;
	qreal valueAt(const QPointF& p);
	qreal percentAt(const QPointF& p);
	qreal posAtValue(qreal value);
	void setColor(const QColor& color);
	void setTextColor(const QColor& color);
	int unitSystem;

protected:
	virtual QString textForValue(double value);

	Qt::Orientation orientation;
	QList<DiveLineItem*> ticks;
	QList<DiveTextItem*> labels;
	double min;
	double max;
	double interval;
	double tickSize;
	QColor textColor;
};

class DepthAxis : public DiveCartesianAxis {
protected:
    QString textForValue(double value);
};

class TimeAxis : public DiveCartesianAxis {
protected:
    QString textForValue(double value);
};
#endif