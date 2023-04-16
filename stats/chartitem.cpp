// SPDX-License-Identifier: GPL-2.0
#include "chartitem.h"
#include "statscolors.h"
#include "statsview.h"
#include "core/globals.h"

#include <cmath>
#include <QQuickWindow>
#include <QSGFlatColorMaterial>
#include <QSGImageNode>
#include <QSGRectangleNode>
#include <QSGTexture>
#include <QSGTextureMaterial>

static int selectionOverlayPixelSize = 2;

static int round_up(double f)
{
	return static_cast<int>(ceil(f));
}

ChartItem::ChartItem(StatsView &v, ChartZValue z) :
	dirty(false), prev(nullptr), next(nullptr),
	zValue(z), view(v)
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

static const int scatterItemDiameter = 10;
static const int scatterItemBorder = 1;

ChartScatterItem::ChartScatterItem(StatsView &v, ChartZValue z, const StatsTheme &theme, bool selected) : HideableChartItem(v, z),
	theme(theme),
	positionDirty(false), textureDirty(false),
	highlight(selected ? Highlight::Selected : Highlight::Unselected)
{
	rect.setSize(QSizeF(static_cast<double>(scatterItemDiameter), static_cast<double>(scatterItemDiameter)));
}

ChartScatterItem::~ChartScatterItem()
{
}

static QSGTexture *createScatterTexture(StatsView &view, const QColor &color, const QColor &borderColor)
{
	QImage img(scatterItemDiameter, scatterItemDiameter, QImage::Format_ARGB32);
	img.fill(Qt::transparent);
	QPainter painter(&img);
	painter.setPen(Qt::NoPen);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setBrush(borderColor);
	painter.drawEllipse(0, 0, scatterItemDiameter, scatterItemDiameter);
	painter.setBrush(color);
	painter.drawEllipse(scatterItemBorder, scatterItemBorder,
			    scatterItemDiameter - 2 * scatterItemBorder,
			    scatterItemDiameter - 2 * scatterItemBorder);
	return view.w()->createTextureFromImage(img, QQuickWindow::TextureHasAlphaChannel);
}

QSGTexture *ChartScatterItem::getTexture() const
{
	switch (highlight) {
	default:
	case Highlight::Unselected:
		return theme.scatterItemTexture;
	case Highlight::Selected:
		return theme.scatterItemSelectedTexture;
	case Highlight::Highlighted:
		return theme.scatterItemHighlightedTexture;
	}
}

void ChartScatterItem::render()
{
	if (!theme.scatterItemTexture) {
		theme.scatterItemTexture = register_global(createScatterTexture(view, theme.fillColor, theme.borderColor));
		theme.scatterItemSelectedTexture = register_global(createScatterTexture(view, theme.selectedColor, theme.selectedBorderColor));
		theme.scatterItemHighlightedTexture = register_global(createScatterTexture(view, theme.highlightedColor, theme.highlightedBorderColor));
	}
	if (!node) {
		createNode(view.w()->createImageNode());
		view.addQSGNode(node.get(), zValue);
		textureDirty = positionDirty = true;
	}
	updateVisible();
	if (textureDirty) {
		node->node->setTexture(getTexture());
		textureDirty = false;
	}
	if (positionDirty) {
		node->node->setRect(rect);
		positionDirty = false;
	}
}

void ChartScatterItem::setPos(QPointF pos)
{
	pos -= QPointF(scatterItemDiameter / 2.0, scatterItemDiameter / 2.0);
	rect.moveTopLeft(pos);
	positionDirty = true;
	markDirty();
}

static double squareDist(const QPointF &p1, const QPointF &p2)
{
	QPointF diff = p1 - p2;
	return QPointF::dotProduct(diff, diff);
}

bool ChartScatterItem::contains(QPointF point) const
{
	return squareDist(point, rect.center()) <= (scatterItemDiameter / 2.0) * (scatterItemDiameter / 2.0);
}

// For rectangular selections, we are more crude: simply check whether the center is in the selection.
bool ChartScatterItem::inRect(const QRectF &selection) const
{
	return selection.contains(rect.center());
}

void ChartScatterItem::setHighlight(Highlight highlightIn)
{
	if (highlight == highlightIn)
		return;
	highlight = highlightIn;
	textureDirty = true;
	markDirty();
}

QRectF ChartScatterItem::getRect() const
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

ChartTextItem::ChartTextItem(StatsView &v, ChartZValue z, const QFont &f, const QString &text) :
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

ChartPieItem::ChartPieItem(StatsView &v, ChartZValue z, const StatsTheme &theme, double borderWidth) : ChartPixmapItem(v, z),
	theme(theme),
	borderWidth(borderWidth)
{
}

static QBrush makeBrush(QColor fill, bool selected, const StatsTheme &theme)
{
	if (!selected)
		return QBrush(fill);
	QImage img(2 * selectionOverlayPixelSize, 2 * selectionOverlayPixelSize, QImage::Format_ARGB32);
	img.fill(fill);
	for (int x = 0; x < selectionOverlayPixelSize; ++x) {
		for (int y = 0; y < selectionOverlayPixelSize; ++y) {
			img.setPixelColor(x, y, theme.selectionOverlayColor);
			img.setPixelColor(x + selectionOverlayPixelSize, y + selectionOverlayPixelSize,
					  theme.selectionOverlayColor);
		}
	}
	return QBrush(img);
}

void ChartPieItem::drawSegment(double from, double to, QColor fill, QColor border, bool selected)
{
	painter->setPen(QPen(border, borderWidth));
	painter->setBrush(makeBrush(fill, selected, theme));
	// For whatever obscure reason, angles of pie pieces are given as 16th of a degree...?
	// Angles increase CCW, whereas pie charts usually are read CW. Therfore, startAngle
	// is dervied from "from" and subtracted from the origin angle at 12:00.
	int startAngle = 90 * 16 - static_cast<int>(round(to * 360.0 * 16.0));
	int spanAngle = static_cast<int>(round((to - from) * 360.0 * 16.0));
	QRectF drawRect(QPointF(0.0, 0.0), rect.size());
	painter->drawPie(drawRect, startAngle, spanAngle);
	setTextureDirty();
}

void ChartPieItem::resize(QSizeF size)
{
	ChartPixmapItem::resize(size);
	img->fill(Qt::transparent);
}

ChartLineItemBase::ChartLineItemBase(StatsView &v, ChartZValue z, QColor color, double width) : HideableChartItem(v, z),
	color(color), width(width), positionDirty(false), materialDirty(false)
{
}

ChartLineItemBase::~ChartLineItemBase()
{
}

void ChartLineItemBase::setLine(QPointF fromIn, QPointF toIn)
{
	from = fromIn;
	to = toIn;
	positionDirty = true;
	markDirty();
}

// Helper function to set points
void setPoint(QSGGeometry::Point2D &v, const QPointF &p)
{
	v.set(static_cast<float>(p.x()), static_cast<float>(p.y()));
}

void setPoint(QSGGeometry::TexturedPoint2D &v, const QPointF &p, const QPointF &t)
{
	v.set(static_cast<float>(p.x()), static_cast<float>(p.y()),
	      static_cast<float>(t.x()), static_cast<float>(t.y()));
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
		setPoint(vertices[0], from);
		setPoint(vertices[1], QPointF(from.x(), to.y()));
		setPoint(vertices[2], to);
		setPoint(vertices[3], QPointF(to.x(), from.y()));
		setPoint(vertices[4], from);
		node->markDirty(QSGNode::DirtyGeometry);
	}

	if (materialDirty) {
		material->setColor(color);
		node->markDirty(QSGNode::DirtyMaterial);
	}

	positionDirty = materialDirty = false;
}

ChartBarItem::ChartBarItem(StatsView &v, ChartZValue z, const StatsTheme &theme, double borderWidth) : HideableChartItem(v, z),
	theme(theme),
	borderWidth(borderWidth), selected(false),
	positionDirty(false), colorDirty(false), selectedDirty(false)
{
}

ChartBarItem::~ChartBarItem()
{
}

QSGTexture *ChartBarItem::getSelectedTexture() const
{
	if (!theme.selectedTexture) {
		QImage img(2, 2, QImage::Format_ARGB32);
		img.fill(Qt::transparent);
		img.setPixelColor(0, 0, theme.selectionOverlayColor);
		img.setPixelColor(1, 1, theme.selectionOverlayColor);
		theme.selectedTexture = register_global(view.w()->createTextureFromImage(img, QQuickWindow::TextureHasAlphaChannel));
	}
	return theme.selectedTexture;
}

void ChartBarItem::render()
{
	if (!node) {
		createNode(view.w()->createRectangleNode());

		borderGeometry.reset(new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), 5));
		borderGeometry->setDrawingMode(QSGGeometry::DrawLineStrip);
		borderGeometry->setLineWidth(static_cast<float>(borderWidth));
		borderMaterial.reset(new QSGFlatColorMaterial);
		borderNode.reset(new QSGGeometryNode);
		borderNode->setGeometry(borderGeometry.get());
		borderNode->setMaterial(borderMaterial.get());

		node->node->appendChildNode(borderNode.get());
		view.addQSGNode(node.get(), zValue);
		positionDirty = colorDirty = selectedDirty = true;
	}
	updateVisible();

	if (colorDirty) {
		node->node->setColor(color);
		borderMaterial->setColor(borderColor);
		node->node->markDirty(QSGNode::DirtyMaterial);
		borderNode->markDirty(QSGNode::DirtyMaterial);
	}

	if (positionDirty) {
		node->node->setRect(rect);
		auto vertices = borderGeometry->vertexDataAsPoint2D();
		setPoint(vertices[0], rect.topLeft());
		setPoint(vertices[1], rect.topRight());
		setPoint(vertices[2], rect.bottomRight());
		setPoint(vertices[3], rect.bottomLeft());
		setPoint(vertices[4], rect.topLeft());
		node->node->markDirty(QSGNode::DirtyGeometry);
		borderNode->markDirty(QSGNode::DirtyGeometry);
	}

	if (selectedDirty) {
		if (selected) {
			if (!selectionNode) {
				// Create the selection overlay if it didn't exist up to now.
				selectionGeometry.reset(new QSGGeometry(QSGGeometry::defaultAttributes_TexturedPoint2D(), 4));
				selectionGeometry->setDrawingMode(QSGGeometry::DrawTriangleStrip);
				selectionMaterial.reset(new QSGTextureMaterial);
				selectionMaterial->setTexture(getSelectedTexture());
				selectionMaterial->setHorizontalWrapMode(QSGTexture::Repeat);
				selectionMaterial->setVerticalWrapMode(QSGTexture::Repeat);
				selectionNode.reset(new QSGGeometryNode);
				selectionNode->setGeometry(selectionGeometry.get());
				selectionNode->setMaterial(selectionMaterial.get());
			}

			node->node->appendChildNode(selectionNode.get());

			// Update the position of the selection overlay, even if the position didn't change.
			positionDirty = true;
		} else {
			if (selectionNode)
				node->node->removeChildNode(selectionNode.get());
		}
	}

	if (selected && positionDirty) {
		double pixelFactor = 2.0 * selectionOverlayPixelSize; // The texture image is 2x2 pixels.
		auto selectionVertices = selectionGeometry->vertexDataAsTexturedPoint2D();
		selectionNode->markDirty(QSGNode::DirtyGeometry);
		setPoint(selectionVertices[0], rect.topLeft(), QPointF());
		setPoint(selectionVertices[1], rect.topRight(), QPointF(rect.width() / pixelFactor, 0.0));
		setPoint(selectionVertices[2], rect.bottomLeft(), QPointF(0.0, rect.height() / pixelFactor));
		setPoint(selectionVertices[3], rect.bottomRight(), QPointF(rect.width() / pixelFactor, rect.height() / pixelFactor));
	}

	positionDirty = colorDirty = selectedDirty = false;
}

void ChartBarItem::setColor(QColor colorIn, QColor borderColorIn)
{
	if (color == colorIn)
		return;
	color = colorIn;
	borderColor = borderColorIn;
	colorDirty = true;
	markDirty();
}

void ChartBarItem::setRect(const QRectF &rectIn)
{
	if (rect == rectIn)
		return;
	rect = rectIn;
	positionDirty = true;
	markDirty();
}

void ChartBarItem::setSelected(bool selectedIn)
{
	if (selected == selectedIn)
		return;
	selected = selectedIn;
	selectedDirty = true;
	markDirty();
}

QRectF ChartBarItem::getRect() const
{
	return rect;
}

ChartBoxItem::ChartBoxItem(StatsView &v, ChartZValue z, const StatsTheme &theme, double borderWidth) :
	ChartBarItem(v, z, theme, borderWidth)
{
}

ChartBoxItem::~ChartBoxItem()
{
}

void ChartBoxItem::render()
{
	// Remember old dirty values, since ChartBarItem::render() will clear them
	bool oldPositionDirty = positionDirty;
	bool oldColorDirty = colorDirty;
	ChartBarItem::render();		// This will create the base node, so no need to check for that.
	if (!whiskersNode) {
		whiskersGeometry.reset(new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), 10));
		whiskersGeometry->setDrawingMode(QSGGeometry::DrawLines);
		whiskersGeometry->setLineWidth(static_cast<float>(borderWidth));
		whiskersMaterial.reset(new QSGFlatColorMaterial);
		whiskersNode.reset(new QSGGeometryNode);
		whiskersNode->setGeometry(whiskersGeometry.get());
		whiskersNode->setMaterial(whiskersMaterial.get());

		node->node->appendChildNode(whiskersNode.get());
		// If this is the first time, make sure to update the geometry.
		oldPositionDirty = oldColorDirty = true;
	}

	if (oldColorDirty) {
		whiskersMaterial->setColor(borderColor);
		whiskersNode->markDirty(QSGNode::DirtyMaterial);
	}

	if (oldPositionDirty) {
		auto vertices = whiskersGeometry->vertexDataAsPoint2D();
		double left = rect.left();
		double right = rect.right();
		double mid = (left + right) / 2.0;
		// top bar
		setPoint(vertices[0], QPointF(left, max));
		setPoint(vertices[1], QPointF(right, max));
		// top whisker
		setPoint(vertices[2], QPointF(mid, max));
		setPoint(vertices[3], QPointF(mid, rect.top()));
		// bottom bar
		setPoint(vertices[4], QPointF(left, min));
		setPoint(vertices[5], QPointF(right, min));
		// bottom whisker
		setPoint(vertices[6], QPointF(mid, min));
		setPoint(vertices[7], QPointF(mid, rect.bottom()));
		// median indicator
		setPoint(vertices[8], QPointF(left, median));
		setPoint(vertices[9], QPointF(right, median));
		whiskersNode->markDirty(QSGNode::DirtyGeometry);
	}
}

void ChartBoxItem::setBox(const QRectF &rect, double minIn, double maxIn, double medianIn)
{
	min = minIn;
	max = maxIn;
	median = medianIn;
	setRect(rect);
}

QRectF ChartBoxItem::getRect() const
{
	QRectF res = rect;
	res.setTop(min);
	res.setBottom(max);
	return rect;
}
