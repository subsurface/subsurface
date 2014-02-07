#ifndef DIVECARTESIANAXIS_H
#define DIVECARTESIANAXIS_H

#include <QObject>
#include <QGraphicsLineItem>

class QPropertyAnimation;
class DiveTextItem;
class DiveLineItem;
class DivePlotDataModel;

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
	void setup(double minimum, double maximum, double interval, Orientation o, qreal tickSize, const QPointF& pos);
	void setMinimum(double minimum);
	void setMaximum(double maximum);
	void setTickInterval(double interval);
	void setOrientation(Orientation orientation);
	void setTickSize(qreal size);
	double minimum() const;
	double maximum() const;
	qreal valueAt(const QPointF& p);
	qreal percentAt(const QPointF& p);
	qreal posAtValue(qreal value);
	void setColor(const QColor& color);
	void setTextColor(const QColor& color);
	void setShowTicks(bool show);
	void setShowText(bool show);
	void animateChangeLine(const QLineF& newLine);
	int unitSystem;
public slots:
	virtual void updateTicks();
signals:
	void sizeChanged();
	void maxChanged();
protected:
	virtual QString textForValue(double value);
	virtual QColor colorForValue(double value);
	Orientation orientation;
	QList<DiveTextItem*> labels;
	double min;
	double max;
	double interval;
	double tickSize;
	QColor textColor;
	bool showTicks;
	bool showText;
};

class DepthAxis : public DiveCartesianAxis {
	Q_OBJECT
public:
	DepthAxis();
protected:
	QString textForValue(double value);
	QColor colorForValue(double value);
private slots:
	void settingsChanged();
private:
	bool showWithPPGraph;
};

class TimeAxis : public DiveCartesianAxis {
	Q_OBJECT
public:
	virtual void updateTicks();
protected:
	QString textForValue(double value);
	QColor colorForValue(double value);
};

class TemperatureAxis : public DiveCartesianAxis{
	Q_OBJECT
protected:
	QString textForValue(double value);
};

class PartialGasPressureAxis : public DiveCartesianAxis{
	Q_OBJECT
public:
	PartialGasPressureAxis();
	void setModel(DivePlotDataModel *model);
public slots:
	void preferencesChanged();
private:
	DivePlotDataModel *model;
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
