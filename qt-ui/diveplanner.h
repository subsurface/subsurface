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
    virtual void showEvent(QShowEvent* event);
    virtual void resizeEvent(QResizeEvent* event);

    void clear_generated_deco();
	void create_deco_stop();

private:
    DivePlanner(QWidget* parent = 0);
	QList<QGraphicsLineItem*> lines;
	QList<QGraphicsEllipseItem*> handles;
};
#endif
