#ifndef DIVEPLANNER_H
#define DIVEPLANNER_H

#include <QGraphicsView>
#include <QGraphicsPathItem>

class DiveHandler : public QGraphicsEllipseItem{
public:
    DiveHandler();

	QGraphicsLineItem *from;
	QGraphicsLineItem *to;
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
    void clear_generated_deco();
	void create_deco_stop();
	bool isPointOutOfBoundaries(QPointF point);

private:
    DivePlanner(QWidget* parent = 0);
	QList<QGraphicsLineItem*> lines;
	QList<DiveHandler *> handles;
	QGraphicsLineItem *verticalLine;
	QGraphicsLineItem *horizontalLine;
};
#endif
