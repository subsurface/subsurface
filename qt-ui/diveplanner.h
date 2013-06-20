#ifndef DIVEPLANNER_H
#define DIVEPLANNER_H

#include <QGraphicsView>
#include <QGraphicsPathItem>

class DivePlanner : public QGraphicsView {
	Q_OBJECT
public:
	static DivePlanner *instance();
protected:
    virtual void mouseDoubleClickEvent(QMouseEvent* event);
	
private:
    DivePlanner(QWidget* parent = 0);
};
#endif
