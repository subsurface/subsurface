// SPDX-License-Identifier: GPL-2.0
#ifndef ROUNDRECTITEM_H
#define ROUNDRECTITEM_H

#include <QGraphicsRectItem>

class RoundRectItem : public QGraphicsRectItem {
public:
	RoundRectItem(double radius, QGraphicsItem *parent);
	RoundRectItem(double radius);
private:
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
	double radius;
};

#endif
