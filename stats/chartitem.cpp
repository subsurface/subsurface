// SPDX-License-Identifier: GPL-2.0
#include "chartitem.h"
#include "statsview.h"

#include <cmath>
#include <QQuickWindow>
#include <QSGImageNode>
#include <QSGTexture>

static int round_up(double f)
{
	return static_cast<int>(ceil(f));
}

ChartItem::ChartItem(StatsView &v, ChartZValue z) :
	dirty(false), zValue(z), view(v), positionDirty(false), textureDirty(false)
{
}

ChartItem::~ChartItem()
{
	painter.reset(); // Make sure to destroy painter before image that is painted on
	view.unregisterChartItem(this);
}

QSizeF ChartItem::sceneSize() const
{
	return view.size();
}

void ChartItem::setTextureDirty()
{
	textureDirty = true;
	dirty = true;
}

void ChartItem::setPositionDirty()
{
	positionDirty = true;
	dirty = true;
}

void ChartItem::render()
{
	if (!dirty)
		return;
	if (!node) {
		node.reset(view.w()->createImageNode());
		view.addQSGNode(node.get(), zValue);
	}
	if (!img) {
		resize(QSizeF(1,1));
		img->fill(Qt::transparent);
	}
	if (textureDirty) {
		texture.reset(view.w()->createTextureFromImage(*img, QQuickWindow::TextureHasAlphaChannel));
		node->setTexture(texture.get());
		textureDirty = false;
	}
	if (positionDirty) {
		node->setRect(rect);
		positionDirty = false;
	}
	dirty = false;
}

void ChartItem::resize(QSizeF size)
{
	painter.reset();
	img.reset(new QImage(round_up(size.width()), round_up(size.height()), QImage::Format_ARGB32));
	painter.reset(new QPainter(img.get()));
	painter->setRenderHint(QPainter::Antialiasing);
	rect.setSize(size);
	setTextureDirty();
}

void ChartItem::setPos(QPointF pos)
{
	rect.moveTopLeft(pos);
	setPositionDirty();
}

QRectF ChartItem::getRect() const
{
	return rect;
}

ChartRectItem::ChartRectItem(StatsView &v, ChartZValue z,
			     const QPen &pen, const QBrush &brush, double radius) : ChartItem(v, z),
	pen(pen), brush(brush), radius(radius)
{
}

ChartRectItem::~ChartRectItem()
{
}

void ChartRectItem::resize(QSizeF size)
{
	ChartItem::resize(size);
	img->fill(Qt::transparent);
	painter->setPen(pen);
	painter->setBrush(brush);
	QSize imgSize = img->size();
	int width = pen.width();
	QRect rect(width / 2, width / 2, imgSize.width() - width, imgSize.height() - width);
	painter->drawRoundedRect(rect, radius, radius, Qt::AbsoluteSize);
}
