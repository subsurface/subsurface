// SPDX-License-Identifier: GPL-2.0
#include "chartitem.h"
#include "chartitemhelper.h"
#include "chartitem_private.h"

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

void ChartItem::drag(QPointF)
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
	scale(1.0), positionDirty(false), textureDirty(false)
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
	doRearrange();
	if (!node) {
		createNode(view.w()->createImageNode());
		addNodeToView();
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

// Scale size and round to integer (because non-integers give strange artifacts for me).
static QSizeF scaleSize(const QSizeF &s, double scale)
{
	return { round(s.width() * scale),
		 round(s.height() * scale) };
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
	rect.setSize(scaleSize(size, scale));
	setPositionDirty(); // position includes the size.
	setTextureDirty();
}

void ChartPixmapItem::drag(QPointF pos)
{
	setPos(pos);
}

void ChartPixmapItem::setPos(QPointF pos)
{
	rect.moveTopLeft(pos);
	setPositionDirty();
}

void ChartPixmapItem::setScale(double scaleIn)
{
	scale = scaleIn;
	if (img) {
		rect.setSize(scaleSize(img->size(), scale));
		setPositionDirty(); // position includes the size.
	}
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
	if (!painter)
		return;
	img->fill(Qt::transparent);
	painter->setPen(pen);
	painter->setBrush(brush);
	QSize imgSize = img->size();
	int width = pen.width();
	QRect rect(width / 2, width / 2, imgSize.width() - width, imgSize.height() - width);
	painter->drawRoundedRect(rect, radius, radius, Qt::AbsoluteSize);
}

AnimatedChartRectItem::~AnimatedChartRectItem()
{
}

void AnimatedChartRectItem::setPixmap(const QPixmap &p, int animSpeed)
{
	if (animSpeed <= 0) {
		resize(p.size());
		if (painter)
			painter->drawPixmap(0, 0, p, 0, 0, p.width(), p.height());
		setTextureDirty();
		return;
	}
	pixmap = p;
	originalSize = img ? img->size() : QSize(1,1);
}

static int mid(int from, int to, double fraction)
{
	return fraction == 1.0 ? to
			       : lrint(from + (to - from) * fraction);
}

void AnimatedChartRectItem::anim(double fraction)
{
	QSize s(mid(originalSize.width(), pixmap.width(), fraction),
		mid(originalSize.height(), pixmap.height(), fraction));
	resize(s);
	if (painter)
		painter->drawPixmap(0, 0, pixmap, 0, 0, s.width(), s.height());
	setTextureDirty();
}

ChartDiskItem::ChartDiskItem(ChartView &v, size_t z, const QPen &pen, const QBrush &brush, bool dragable) :
	ChartPixmapItem(v, z, dragable),
	pen(pen), brush(brush)
{
}

ChartDiskItem::~ChartDiskItem()
{
}

void ChartDiskItem::resize(double radius)
{
	ChartPixmapItem::resize(QSizeF(2.0 * radius, 2.0 * radius));
	if (!painter)
		return;
	img->fill(Qt::transparent);
	painter->setPen(pen);
	painter->setBrush(brush);
	QSize imgSize = img->size();
	int width = pen.width();
	QRect rect(width / 2, width / 2, imgSize.width() - width, imgSize.height() - width);
	painter->drawEllipse(rect);
}

// Moves the _center_ of the disk to given position.
void ChartDiskItem::setPos(QPointF pos)
{
	rect.moveCenter(pos);
	setPositionDirty();
}

QPointF ChartDiskItem::getPos() const
{
	return rect.center();
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
	doRearrange();
	if (!node) {
		geometry.reset(new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), 2));
		geometry->setDrawingMode(QSGGeometry::DrawLines);
		material.reset(new QSGFlatColorMaterial);
		createNode();
		node->setGeometry(geometry.get());
		node->setMaterial(material.get());
		addNodeToView();
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
	doRearrange();
	if (!node) {
		geometry.reset(new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), 5));
		geometry->setDrawingMode(QSGGeometry::DrawLineStrip);
		material.reset(new QSGFlatColorMaterial);
		createNode();
		node->setGeometry(geometry.get());
		node->setMaterial(material.get());
		addNodeToView();
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
