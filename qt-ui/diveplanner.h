#ifndef DIVEPLANNER_H
#define DIVEPLANNER_H

#include <QGraphicsView>
#include <QGraphicsPathItem>
#include <QDialog>

class Button : public QObject, public QGraphicsRectItem {
	Q_OBJECT
public:
	explicit Button(QObject* parent = 0);
	void setText(const QString& text);
	void setPixmap(const QPixmap& pixmap);

protected:
	virtual void mousePressEvent(QGraphicsSceneMouseEvent* event);
signals:
	void clicked();
private:
	QGraphicsPixmapItem *icon;
	QGraphicsSimpleTextItem *text;
};

class DiveHandler : public QGraphicsEllipseItem{
public:
	DiveHandler();
	QGraphicsLineItem *from;
	QGraphicsLineItem *to;
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

	void createDecoStops();
	bool isPointOutOfBoundaries(const QPointF& point);
	void deleteTemporaryDivePlan(struct divedatapoint* dp);

	qreal fromPercent(qreal percent, Qt::Orientation orientation);
private slots:
	void increaseTime();
	void increaseDepth();
	void okClicked();
	void cancelClicked();

private:
	void moveActiveHandler(const QPointF& pos);

	/* This are the lines of the plotted dive. */
	QList<QGraphicsLineItem*> lines;

	/* This is the user-entered handles. */
	QList<DiveHandler *> handles;

	/* those are the lines that follows the mouse. */
	QGraphicsLineItem *verticalLine;
	QGraphicsLineItem *horizontalLine;

	/* This is the handler that's being dragged. */
	DiveHandler *activeDraggedHandler;

	// helper to save the positions where the drag-handler is valid.
	QPointF lastValidPos;

	/* this is the background of the dive, the blue-gradient. */
	QGraphicsPolygonItem *diveBg;

	/* This is the bottom ruler - the x axis, and it's associated text */
	Ruler *timeLine;
	QGraphicsSimpleTextItem *timeString;

	/* this is the left ruler, the y axis, and it's associated text. */
	Ruler *depthLine;
	QGraphicsSimpleTextItem *depthString;

	/* Buttons */
	Button *plusTime;  // adds 10 minutes to the time ruler.
	Button *plusDepth; // adds 10 meters to the depth ruler.
	Button *lessTime;  // remove 10 minutes to the time ruler.
	Button *lessDepth; // remove 10 meters to the depth ruler.
	Button *okBtn;     // accepts, and creates a new dive based on the plan.
	Button *cancelBtn; // rejects, and clears the dive plan.
};

#endif
