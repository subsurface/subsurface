// SPDX-License-Identifier: GPL-2.0
#ifndef DIVEHANDLER_HPP
#define DIVEHANDLER_HPP

#include <QGraphicsPathItem>
#include <QElapsedTimer>

struct dive;

class DiveHandler : public QObject, public QGraphicsEllipseItem {
	Q_OBJECT
public:
	DiveHandler(const struct dive *d);

protected:
	void contextMenuEvent(QGraphicsSceneContextMenuEvent *event);
	void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
	void mousePressEvent(QGraphicsSceneMouseEvent *event);
	void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
signals:
	void moved();
	void clicked();
	void released();
private:
	int parentIndex();
public
slots:
	void selfRemove();
	void changeGas();
private:
	const struct dive *dive;
	QElapsedTimer t;
};

#endif
