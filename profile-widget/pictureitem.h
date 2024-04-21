// SPDX-License-Identifier: GPL-2.0
// Shows a picture or video on the profile
#ifndef PICTUREMAPITEM_H
#define PICTUREMAPITEM_H

#include "qt-quick/chartitem.h"
#include <QString>
#include <QRectF>

class QPixmap;

class PictureItem : public ChartPixmapItem {
public:
	PictureItem(ChartView &view, double dpr);
	~PictureItem();
	void setPixmap(const QPixmap &pix);
	void setFileUrl(const QString &s);
	double right() const;
	double left() const;
	bool underMouse(QPointF pos) const;
	bool removeIconUnderMouse(QPointF pos) const;
	void grow(int animSpeed);
	void shrink(int animSpeed);
	void anim(double progress);
private:
	double dpr;
	QRectF removeIconRect;
	double fromScale, toScale; // For animation

	void initAnimation(double scale, int animSpeed);
};

#endif
