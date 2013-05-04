#ifndef PROFILEGRAPHICS_H
#define PROFILEGRAPHICS_H

#include <QGraphicsView>

class ProfileGraphicsView : public QGraphicsView {
Q_OBJECT
public:
	ProfileGraphicsView(QWidget* parent = 0);
	void plot(struct dive *d);

protected:
	void resizeEvent(QResizeEvent *event);

};

#endif
