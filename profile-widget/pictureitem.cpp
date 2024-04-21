// SPDX-License-Identifier: GPL-2.0
#include "pictureitem.h"
#include "zvalues.h"
#include <cmath>

static constexpr double scaleFactor = 0.2;
static constexpr double shadowSize = 5.0;
static constexpr double removeIconSize = 20.0;

PictureItem::PictureItem(ChartView &view, double dpr) :
	ChartPixmapItem(view, ProfileZValue::Pictures, false),
	dpr(dpr)
{
	setScale(scaleFactor); // Start small
}

PictureItem::~PictureItem()
{
}

void PictureItem::setPixmap(const QPixmap &picture)
{
	static QPixmap removeIcon = QPixmap(":list-remove-icon");

	int shadowSizeInt = lrint(shadowSize * dpr);
	resize(QSizeF(picture.width() + shadowSizeInt, picture.height() + shadowSizeInt)); // initializes canvas
	img->fill(Qt::transparent);
	painter->setPen(Qt::NoPen);
	painter->setBrush(QColor(Qt::lightGray));
	painter->drawRect(shadowSizeInt, shadowSizeInt, picture.width(), picture.height());
	painter->drawPixmap(0, 0, picture, 0, 0, picture.width(), picture.height());

	int removeIconSizeInt = lrint(::removeIconSize * dpr);
	QPixmap icon = removeIcon.scaledToWidth(removeIconSizeInt, Qt::SmoothTransformation);
	removeIconRect = QRect(picture.width() - icon.width(), 0, icon.width(), icon.height());
	painter->drawPixmap(picture.width() - icon.width(), 0, icon, 0, 0, icon.width(), icon.height());
}

double PictureItem::left() const
{
	return rect.left();
}

double PictureItem::right() const
{
	return rect.right();
}

bool PictureItem::underMouse(QPointF pos) const
{
	return rect.contains(pos);
}

bool PictureItem::removeIconUnderMouse(QPointF pos) const
{
	if (!underMouse(pos))
		return false;
	QPointF pos_rel = (pos - rect.topLeft()) / scale;
	return removeIconRect.contains(pos_rel);
}

void PictureItem::initAnimation(double scale, int animSpeed)
{
	if (animSpeed <= 0)
		return setScale(1.0);

	fromScale = this->scale;
	toScale = scale;
}

void PictureItem::grow(int animSpeed)
{
	initAnimation(1.0, animSpeed);
}

void PictureItem::shrink(int animSpeed)
{
	initAnimation(scaleFactor, animSpeed);
}

static double mid(double from, double to, double fraction)
{
	return fraction == 1.0 ? to
			       : from + (to - from) * fraction;
}

void PictureItem::anim(double progress)
{
	setScale(mid(fromScale, toScale, progress));
	setPositionDirty();
}
