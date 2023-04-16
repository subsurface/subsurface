// SPDX-License-Identifier: GPL-2.0
#include "chartitem.h"
#include "statscolors.h"
#include "statsview.h"
#include "core/globals.h"
#include "qt-quick/chartitemhelper.h"

#include <cmath>
#include <QQuickWindow>
#include <QSGFlatColorMaterial>
#include <QSGImageNode>
#include <QSGRectangleNode>
#include <QSGTexture>
#include <QSGTextureMaterial>

static int selectionOverlayPixelSize = 2;

static const int scatterItemDiameter = 10;
static const int scatterItemBorder = 1;

ChartScatterItem::ChartScatterItem(ChartView &v, size_t z, const StatsTheme &theme, bool selected) : HideableChartItem(v, z),
	theme(theme),
	positionDirty(false), textureDirty(false),
	highlight(selected ? Highlight::Selected : Highlight::Unselected)
{
	rect.setSize(QSizeF(static_cast<double>(scatterItemDiameter), static_cast<double>(scatterItemDiameter)));
}

ChartScatterItem::~ChartScatterItem()
{
}

static QSGTexture *createScatterTexture(ChartView &view, const QColor &color, const QColor &borderColor)
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

ChartPieItem::ChartPieItem(ChartView &v, size_t z, const StatsTheme &theme, double borderWidth) : ChartPixmapItem(v, z),
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

ChartLineItemBase::ChartLineItemBase(ChartView &v, size_t z, QColor color, double width) : HideableChartItem(v, z),
	color(color), width(width), positionDirty(false), materialDirty(false)
{
}

ChartBarItem::ChartBarItem(ChartView &v, size_t z, const StatsTheme &theme, double borderWidth) : HideableChartItem(v, z),
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

ChartBoxItem::ChartBoxItem(ChartView &v, size_t z, const StatsTheme &theme, double borderWidth) :
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
