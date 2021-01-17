// SPDX-License-Identifier: GPL-2.0
#include "chartitem.h"
#include "statsview.h"

#include <cmath>
#include <QQuickWindow>
#include <QSGFlatColorMaterial>
#include <QSGImageNode>
#include <QSGRectangleNode>
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

ChartPixmapItem::ChartPixmapItem(StatsView &v, ChartZValue z) : HideableChartItem(v, z),
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
		createNode(view.w()->createImageNode());
		view.addQSGNode(node.get(), zValue);
	}
	if (!img) {
		resize(QSizeF(1,1));
		img->fill(Qt::transparent);
	}
	if (textureDirty) {
		texture.reset(view.w()->createTextureFromImage(*img, QQuickWindow::TextureHasAlphaChannel));
		node->node->setTexture(texture.get());
		textureDirty = false;
	}
	if (positionDirty) {
		node->node->setRect(rect);
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

ChartTextItem::ChartTextItem(StatsView &v, ChartZValue z, const QFont &f, const std::vector<QString> &text, bool center) :
	ChartPixmapItem(v, z), f(f), center(center)
{
	QFontMetrics fm(f);
	double totalWidth = 1.0;
	fontHeight = static_cast<double>(fm.height());
	double totalHeight = std::max(1.0, static_cast<double>(text.size()) * fontHeight);

	items.reserve(text.size());
	for (const QString &s: text) {
		double w = fm.size(Qt::TextSingleLine, s).width();
		items.push_back({ s, w });
		if (w > totalWidth)
			totalWidth = w;
	}
	resize(QSizeF(totalWidth, totalHeight));
}

void ChartTextItem::setColor(const QColor &c)
{
	img->fill(Qt::transparent);
	double y = 0.0;
	painter->setPen(QPen(c));
	painter->setFont(f);
	double totalWidth = getRect().width();
	for (const auto &[s, w]: items) {
		double x = center ? round((totalWidth - w) / 2.0) : 0.0;
		QRectF rect(x, y, w, fontHeight);
		painter->drawText(rect, s);
		y += fontHeight;
	}
	setTextureDirty();
}

ChartLineItem::ChartLineItem(StatsView &v, ChartZValue z, QColor color, double width) : HideableChartItem(v, z),
	color(color), width(width), positionDirty(false), materialDirty(false)
{
}

ChartLineItem::~ChartLineItem()
{
}

// Helper function to set points
void setPoint(QSGGeometry::Point2D &v, const QPointF &p)
{
	v.set(static_cast<float>(p.x()), static_cast<float>(p.y()));
}

void ChartLineItem::render()
{
	if (!node) {
		geometry.reset(new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), 2));
		geometry->setDrawingMode(QSGGeometry::DrawLines);
		material.reset(new QSGFlatColorMaterial);
		createNode();
		node->setGeometry(geometry.get());
		node->setMaterial(material.get());
		view.addQSGNode(node.get(), zValue);
		positionDirty = materialDirty = true;
	}

	if (positionDirty) {
		// Attention: width is a geometry property and therefore handled by position dirty!
		geometry->setLineWidth(static_cast<float>(width));
		auto vertices = geometry->vertexDataAsPoint2D();
		setPoint(vertices[0], from);
		setPoint(vertices[1], to);
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

ChartBarItem::ChartBarItem(StatsView &v, ChartZValue z, double borderWidth, bool horizontal) : HideableChartItem(v, z),
	borderWidth(borderWidth), horizontal(horizontal),
	positionDirty(false), colorDirty(false)
{
}

ChartBarItem::~ChartBarItem()
{
}

void ChartBarItem::render()
{
	if (!node) {
		createNode(view.w()->createRectangleNode());

		borderGeometry.reset(new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), 4));
		borderGeometry->setDrawingMode(QSGGeometry::DrawLineLoop);
		borderGeometry->setLineWidth(static_cast<float>(borderWidth));
		borderMaterial.reset(new QSGFlatColorMaterial);
		borderNode.reset(new QSGGeometryNode);
		borderNode->setGeometry(borderGeometry.get());
		borderNode->setMaterial(borderMaterial.get());

		node->node->appendChildNode(borderNode.get());
		view.addQSGNode(node.get(), zValue);
		positionDirty = colorDirty = true;
	}

	if (colorDirty) {
		node->node->setColor(color);
		borderMaterial->setColor(borderColor);
		node->node->markDirty(QSGNode::DirtyMaterial);
		borderNode->markDirty(QSGNode::DirtyMaterial);
	}

	if (positionDirty) {
		node->node->setRect(rect);
		auto vertices = borderGeometry->vertexDataAsPoint2D();
		if (horizontal) {
			setPoint(vertices[0], rect.topLeft());
			setPoint(vertices[1], rect.topRight());
			setPoint(vertices[2], rect.bottomRight());
			setPoint(vertices[3], rect.bottomLeft());
		} else {
			setPoint(vertices[0], rect.bottomLeft());
			setPoint(vertices[1], rect.topLeft());
			setPoint(vertices[2], rect.topRight());
			setPoint(vertices[3], rect.bottomRight());
		}
		node->node->markDirty(QSGNode::DirtyGeometry);
		borderNode->markDirty(QSGNode::DirtyGeometry);
	}

	positionDirty = colorDirty = false;
}

void ChartBarItem::setColor(QColor colorIn, QColor borderColorIn)
{
	color = colorIn;
	borderColor = borderColorIn;
	colorDirty = true;
	view.registerDirtyChartItem(*this);
}

void ChartBarItem::setRect(const QRectF &rectIn)
{
	rect = rectIn;
	positionDirty = true;
	view.registerDirtyChartItem(*this);
}

QRectF ChartBarItem::getRect() const
{
	return rect;
}
