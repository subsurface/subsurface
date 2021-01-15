// SPDX-License-Identifier: GPL-2.0
#include "chartitem.h"
#include "statsview.h"

#include <cmath>
#include <QQuickWindow>
#include <QSGFlatColorMaterial>
#include <QSGImageNode>
#include <QSGTexture>

static int round_up(double f)
{
	return static_cast<int>(ceil(f));
}

ChartItem::ChartItem(StatsView &v, ChartZValue z) :
	dirty(false), dirtyPrev(nullptr), dirtyNext(nullptr),
	zValue(z), view(v)
{
}

ChartItem::~ChartItem()
{
	if (dirty)
		view.unregisterDirtyChartItem(*this);
}

QSizeF ChartItem::sceneSize() const
{
	return view.size();
}

ChartPixmapItem::ChartPixmapItem(StatsView &v, ChartZValue z) : ChartItem(v, z),
	positionDirty(false), textureDirty(false)
{
}

ChartPixmapItem::~ChartPixmapItem()
{
	painter.reset(); // Make sure to destroy painter before image that is painted on
}

void ChartPixmapItem::setTextureDirty()
{
	textureDirty = true;
	view.registerDirtyChartItem(*this);
}

void ChartPixmapItem::setPositionDirty()
{
	positionDirty = true;
	view.registerDirtyChartItem(*this);
}

void ChartPixmapItem::render()
{
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
}

void ChartPixmapItem::resize(QSizeF size)
{
	painter.reset();
	img.reset(new QImage(round_up(size.width()), round_up(size.height()), QImage::Format_ARGB32));
	painter.reset(new QPainter(img.get()));
	painter->setRenderHint(QPainter::Antialiasing);
	rect.setSize(size);
	setTextureDirty();
}

void ChartPixmapItem::setPos(QPointF pos)
{
	rect.moveTopLeft(pos);
	setPositionDirty();
}

QRectF ChartPixmapItem::getRect() const
{
	return rect;
}

ChartRectItem::ChartRectItem(StatsView &v, ChartZValue z,
			     const QPen &pen, const QBrush &brush, double radius) : ChartPixmapItem(v, z),
	pen(pen), brush(brush), radius(radius)
{
}

ChartRectItem::~ChartRectItem()
{
}

void ChartRectItem::resize(QSizeF size)
{
	ChartPixmapItem::resize(size);
	img->fill(Qt::transparent);
	painter->setPen(pen);
	painter->setBrush(brush);
	QSize imgSize = img->size();
	int width = pen.width();
	QRect rect(width / 2, width / 2, imgSize.width() - width, imgSize.height() - width);
	painter->drawRoundedRect(rect, radius, radius, Qt::AbsoluteSize);
}

ChartLineItem::ChartLineItem(StatsView &v, ChartZValue z, QColor color, double width) : ChartItem(v, z),
	color(color), width(width), positionDirty(false), materialDirty(false)
{
}

ChartLineItem::~ChartLineItem()
{
}

void ChartLineItem::render()
{
	if (!node) {
		geometry.reset(new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), 2));
		geometry->setDrawingMode(QSGGeometry::DrawLines);
		material.reset(new QSGFlatColorMaterial);
		node.reset(new QSGGeometryNode);
		node->setGeometry(geometry.get());
		node->setMaterial(material.get());
		view.addQSGNode(node.get(), zValue);
		positionDirty = materialDirty = true;
	}

	if (positionDirty) {
		// Attention: width is a geometry property and therefore handled by position dirty!
		geometry->setLineWidth(static_cast<float>(width));
		auto vertices = geometry->vertexDataAsPoint2D();
		vertices[0].set(static_cast<float>(from.x()), static_cast<float>(from.y()));
		vertices[1].set(static_cast<float>(to.x()), static_cast<float>(to.y()));
		node->markDirty(QSGNode::DirtyGeometry);
	}

	if (materialDirty) {
		material->setColor(color);
		node->markDirty(QSGNode::DirtyMaterial);
	}

	positionDirty = materialDirty = false;
}

void ChartLineItem::setLine(QPointF fromIn, QPointF toIn)
{
	from = fromIn;
	to = toIn;
	positionDirty = true;
	view.registerDirtyChartItem(*this);
}
