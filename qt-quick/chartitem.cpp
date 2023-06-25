// SPDX-License-Identifier: GPL-2.0
#include "chartitem.h"
#include "chartitemhelper.h"
#include "chartview.h"

#include <cmath>
#include <QGraphicsScene>
#include <QQuickWindow>
#include <QSGFlatColorMaterial>
#include <QSGImageNode>

ChartItem::ChartItem(ChartView &v, size_t z, bool dragable) :
	dirty(false), prev(nullptr), next(nullptr),
	zValue(z), dragable(dragable), view(v)
{
	// Register before the derived constructors run, so that the
	// derived classes can mark the item as dirty in the constructor.
	v.registerChartItem(*this);
}

ChartItem::~ChartItem()
{
}

QSizeF ChartItem::sceneSize() const
{
	return view.size();
}

void ChartItem::markDirty()
{
	view.registerDirtyChartItem(*this);
}

QRectF ChartItem::getRect() const
{
	return rect;
}

void ChartItem::setPos(QPointF)
{
}

void ChartItem::stopDrag(QPointF pos)
{
}

static int round_up(double f)
{
	return static_cast<int>(ceil(f));
}

ChartPixmapItem::ChartPixmapItem(ChartView &v, size_t z, bool dragable) : HideableChartItem(v, z, dragable),
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
	markDirty();
}

void ChartPixmapItem::setPositionDirty()
{
	positionDirty = true;
	markDirty();
}

void ChartPixmapItem::render()
{
	if (!node) {
		createNode(view.w()->createImageNode());
		view.addQSGNode(node.get(), zValue);
	}
	updateVisible();

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
	QSize s_int(round_up(size.width()), round_up(size.height()));
	if (img && s_int == img->size())
		return;
	painter.reset();
	img.reset(new QImage(s_int, QImage::Format_ARGB32));
	if (!img->isNull()) {
		painter.reset(new QPainter(img.get()));
		painter->setRenderHint(QPainter::Antialiasing);
	}
	rect.setSize(size);
	setPositionDirty(); // position includes the size.
	setTextureDirty();
}

void ChartPixmapItem::setPos(QPointF pos)
{
	rect.moveTopLeft(pos);
	setPositionDirty();
}

void ChartGraphicsSceneItem::draw(QSizeF s, QColor background, QGraphicsScene &scene)
{
	resize(s); // Noop if size doesn't change
	if (!painter)
		return;		// Happens if we resize to (0,0)
	img->fill(background);
	scene.render(painter.get(), QRect(QPoint(), img->size()), scene.sceneRect(), Qt::IgnoreAspectRatio);
	setTextureDirty();
}

ChartRectItem::ChartRectItem(ChartView &v, size_t z, const QPen &pen,
			     const QBrush &brush, double radius, bool dragable) :
	ChartPixmapItem(v, z, dragable),
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

ChartTextItem::ChartTextItem(ChartView &v, size_t z, const QFont &f, const std::vector<QString> &text, bool center) :
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

ChartTextItem::ChartTextItem(ChartView &v, size_t z, const QFont &f, const QString &text) :
	ChartTextItem(v, z, f, std::vector<QString>({ text }), true)
{
}

void ChartTextItem::setColor(const QColor &c)
{
	setColor(c, Qt::transparent);
}

void ChartTextItem::setColor(const QColor &c, const QColor &background)
{
	img->fill(background);
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

ChartLineItemBase::~ChartLineItemBase()
{
}

void ChartLineItemBase::setLine(QPointF from, QPointF to)
{
	rect = QRectF(from, to);
	positionDirty = true;
	markDirty();
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
	updateVisible();

	if (positionDirty) {
		// Attention: width is a geometry property and therefore handled by position dirty!
		geometry->setLineWidth(static_cast<float>(width));
		auto vertices = geometry->vertexDataAsPoint2D();
		setPoint(vertices[0], rect.topLeft());
		setPoint(vertices[1], rect.bottomRight());
		node->markDirty(QSGNode::DirtyGeometry);
	}

	if (materialDirty) {
		material->setColor(color);
		node->markDirty(QSGNode::DirtyMaterial);
	}

	positionDirty = materialDirty = false;
}

void ChartRectLineItem::render()
{
	if (!node) {
		geometry.reset(new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), 5));
		geometry->setDrawingMode(QSGGeometry::DrawLineStrip);
		material.reset(new QSGFlatColorMaterial);
		createNode();
		node->setGeometry(geometry.get());
		node->setMaterial(material.get());
		view.addQSGNode(node.get(), zValue);
		positionDirty = materialDirty = true;
	}
	updateVisible();

	if (positionDirty) {
		// Attention: width is a geometry property and therefore handled by position dirty!
		geometry->setLineWidth(static_cast<float>(width));
		auto vertices = geometry->vertexDataAsPoint2D();
		setPoint(vertices[0], rect.topLeft());
		setPoint(vertices[1], rect.bottomLeft());
		setPoint(vertices[2], rect.bottomRight());
		setPoint(vertices[3], rect.topRight());
		setPoint(vertices[4], rect.topLeft());
		node->markDirty(QSGNode::DirtyGeometry);
	}

	if (materialDirty) {
		material->setColor(color);
		node->markDirty(QSGNode::DirtyMaterial);
	}

	positionDirty = materialDirty = false;
}
