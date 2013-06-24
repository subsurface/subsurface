#ifndef DIVEPLANNER_H
#define DIVEPLANNER_H

#include <QGraphicsView>
#include <QGraphicsPathItem>
#include <QDialog>

class Button : public QObject, public QGraphicsPixmapItem {
	Q_OBJECT
public:
	explicit Button(QObject* parent = 0);
protected:
	virtual void mousePressEvent(QGraphicsSceneMouseEvent* event);
signals:
	void clicked();
};

class DiveHandler : public QGraphicsEllipseItem{
public:
	DiveHandler();
	QGraphicsLineItem *from;
	QGraphicsLineItem *to;
	qreal time;
	qreal depth;
	int sec;
	int mm;
};

class Ruler : public QGraphicsLineItem{
public:
	Ruler();
	void setMinimum(double minimum);
	void setMaximum(double maximum);
	void setTickInterval(double interval);
	void setOrientation(Qt::Orientation orientation);
	void updateTicks();
	double minimum() const;
	double maximum() const;
	qreal valueAt(const QPointF& p);
	qreal posAtValue(qreal value);

private:
	Qt::Orientation orientation;
	QList<QGraphicsLineItem*> ticks;
	double min;
	double max;
	double interval;
	double posBegin;
	double posEnd;
};

class DivePlannerGraphics : public QGraphicsView {
	Q_OBJECT
public:
	DivePlannerGraphics(QWidget* parent = 0);
protected:
	virtual void mouseDoubleClickEvent(QMouseEvent* event);
	virtual void showEvent(QShowEvent* event);
	virtual void resizeEvent(QResizeEvent* event);
	virtual void mouseMoveEvent(QMouseEvent* event);
	virtual void mousePressEvent(QMouseEvent* event);
	virtual void mouseReleaseEvent(QMouseEvent* event);

	void clearGeneratedDeco();
	void createDecoStops();
	bool isPointOutOfBoundaries(const QPointF& point);

private slots:
	void increaseTime();
	void increaseDepth();

private:

	void moveActiveHandler(const QPointF& pos);
	QList<QGraphicsLineItem*> lines;
	QList<DiveHandler *> handles;
	QGraphicsLineItem *verticalLine;
	QGraphicsLineItem *horizontalLine;
	DiveHandler *activeDraggedHandler;

	Ruler *timeLine;
	QGraphicsSimpleTextItem *timeString;

	Ruler *depthLine;
	QGraphicsSimpleTextItem *depthString;

	Button *plusTime;
	Button *plusDepth;
	Button *lessTime;
	Button *lessDepth;

};

namespace Ui{
	class DivePlanner;
}

class DivePlanner : public QDialog{
	Q_OBJECT
public:
	static DivePlanner *instance();
	struct dive* getDive();

private:
	DivePlanner();
	Ui::DivePlanner *ui;
};
#endif
