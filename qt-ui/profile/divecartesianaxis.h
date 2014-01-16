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
	enum Orientation{TopToBottom, BottomToTop, LeftToRight, RightToLeft};
	DiveCartesianAxis();
	virtual ~DiveCartesianAxis();
	void setMinimum(double minimum);
	void setMaximum(double maximum);
	void setTickInterval(double interval);
	void setOrientation(Orientation orientation);
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
signals:
	void sizeChanged();
protected:
	virtual QString textForValue(double value);

	Orientation orientation;
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

class TemperatureAxis : public DiveCartesianAxis{
	Q_OBJECT
protected:
	QString textForValue(double value);
};

// This is a try. Maybe the CartesianPlane should have the X and Y
// axis and handle things internally?
class DiveCartesianPlane :public QObject, public QGraphicsRectItem{
	Q_OBJECT
	Q_PROPERTY(QLineF verticalLine READ verticalLine WRITE setVerticalLine)
	Q_PROPERTY(QLineF horizontalLine READ horizontalLine WRITE setHorizontalLine)
public:
	void setLeftAxis(DiveCartesianAxis *axis);
	void setBottomAxis(DiveCartesianAxis *axis);
	void setHorizontalLine(QLineF line);
	void setVerticalLine(QLineF line);
	QLineF horizontalLine() const;
	QLineF verticalLine() const;
public slots:
	void setup();
private:
	DiveCartesianAxis *leftAxis;
	DiveCartesianAxis *bottomAxis;
	QList<DiveLineItem*> verticalLines;
	QList<DiveLineItem*> horizontalLines;
	qreal verticalSize;
	qreal horizontalSize;
};
#endif