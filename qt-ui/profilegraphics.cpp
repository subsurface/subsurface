#include "profilegraphics.h"

#include <QGraphicsScene>
#include <QResizeEvent>

#include <QDebug>

ProfileGraphicsView::ProfileGraphicsView(QWidget* parent) : QGraphicsView(parent)
{
	setScene(new QGraphicsScene());
	scene()->setSceneRect(0,0,100,100);
}

void ProfileGraphicsView::plot(struct dive *d)
{
	qDebug() << "Start the plotting of the dive here.";
}

void ProfileGraphicsView::resizeEvent(QResizeEvent *event)
{
	// Fits the scene's rectangle on the view.
	// I can pass some parameters to this -
	// like Qt::IgnoreAspectRatio or Qt::KeepAspectRatio
	QRectF r = scene()->sceneRect();
	fitInView ( r.x() - 2, r.y() -2, r.width() + 4, r.height() + 4); // do a little bit of spacing;
}
