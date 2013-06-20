#ifndef DIVEPLANNER_H
#define DIVEPLANNER_H

#include <QGraphicsView>
#include <QGraphicsPathItem>

class DiveHandler : public QGraphicsEllipseItem{
public:
    DiveHandler();
	void setTime(qreal t);
	void setDepth(qreal d);

	QGraphicsLineItem *from;
	QGraphicsLineItem *to;
private:
	qreal time;
	qreal depth;
};

class Ruler : public QGraphicsLineItem{
public:
    Ruler();
	void setMinimum(double minimum);
	void setMaximum(double maximum);
	void setTickInterval(double interval);
	void setOrientation(Qt::Orientation orientation);
	void updateTicks();
	qreal valueAt(const QPointF& p);

private:
	Qt::Orientation orientation;
	QList<QGraphicsLineItem*> ticks;
	double min;
	double max;
	double interval;
	double posBegin;
	double posEnd;
};

class DivePlanner : public QGraphicsView {
	Q_OBJECT
public:
	static DivePlanner *instance();
protected:
    virtual void mouseDoubleClickEvent(QMouseEvent* event);
    virtual void showEvent(QShowEvent* event);
    virtual void resizeEvent(QResizeEvent* event);
    virtual void mouseMoveEvent(QMouseEvent* event);
    virtual void mousePressEvent(QMouseEvent* event);
    virtual void mouseReleaseEvent(QMouseEvent* event);

    void clear_generated_deco();
	void create_deco_stop();
	bool isPointOutOfBoundaries(QPointF point);

private:
    DivePlanner(QWidget* parent = 0);
    void moveActiveHandler(QPointF pos);
	QList<QGraphicsLineItem*> lines;
	QList<DiveHandler *> handles;
	QGraphicsLineItem *verticalLine;
	QGraphicsLineItem *horizontalLine;
	DiveHandler *activeDraggedHandler;

	Ruler *timeLine;
	QGraphicsSimpleTextItem *timeString;

	Ruler *depthLine;
	QGraphicsSimpleTextItem *depthString;

};
#endif
