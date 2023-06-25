// SPDX-License-Identifier: GPL-2.0
// Wrappers around QSGImageNode that allow painting onto an image
// and then turning that into a texture to be displayed in a QQuickItem.
#ifndef CHART_ITEM_H
#define CHART_ITEM_H

#include "chartitem_ptr.h"
#include "chartitemhelper.h"

#include <memory>
#include <QPainter>

class ChartView;
class QGraphicsScene;
class QSGGeometry;
class QSGGeometryNode;
class QSGFlatColorMaterial;
class QSGImageNode;
class QSGRectangleNode;
class QSGTexture;
class QSGTextureMaterial;

class ChartItem {
public:
	// Only call on render thread!
	virtual void render() = 0;
	bool dirty;				// If true, call render() when rebuilding the scene
	ChartItem *prev, *next;			// Double linked list of items
	const size_t zValue;
	const bool dragable;			// Item can be dragged with the mouse. Must be set in constructor.
	virtual ~ChartItem();			// Attention: must only be called by render thread.
	QRectF getRect() const;
	virtual void setPos(QPointF pos);	// Called when dragging the item
	virtual void stopDrag(QPointF pos);	// Called when dragging the item finished
protected:
	ChartItem(ChartView &v, size_t z, bool dragable = false);
	QSizeF sceneSize() const;
	ChartView &view;
	void markDirty();
	QRectF rect;
};

template <typename Node>
class HideableChartItem : public ChartItem {
protected:
	HideableChartItem(ChartView &v, size_t z, bool dragable = false);
	std::unique_ptr<Node> node;
	bool visible;
	bool visibleChanged;
	template<class... Args>
	void createNode(Args&&... args);	// Call to create node with visibility flag.
	void updateVisible();			// Must be called by child class to update visibility flag!
public:
	void setVisible(bool visible);
};

// A shortcut for ChartItems based on a hideable proxy item
template <typename Node>
using HideableChartProxyItem = HideableChartItem<HideableQSGNode<QSGProxyNode<Node>>>;

// A chart item that blits a precalculated pixmap onto the scene.
class ChartPixmapItem : public HideableChartProxyItem<QSGImageNode> {
public:
	ChartPixmapItem(ChartView &v, size_t z, bool dragable = false);
	~ChartPixmapItem();

	void setPos(QPointF pos) override;
	void render() override;
protected:
	void resize(QSizeF size);	// Resets the canvas. Attention: image is *unitialized*.
	std::unique_ptr<QPainter> painter;
	std::unique_ptr<QImage> img;
	void setTextureDirty();
	void setPositionDirty();
private:
	bool positionDirty;		// true if the position changed since last render
	bool textureDirty;		// true if the pixmap changed since last render
	std::unique_ptr<QSGTexture> texture;
};

// Renders a QGraphicsScene
class ChartGraphicsSceneItem : public ChartPixmapItem
{
public:
	using ChartPixmapItem::ChartPixmapItem;
	void draw(QSizeF s, QColor background, QGraphicsScene &scene);
};

// Draw a rectangular background after resize. Children are responsible for calling update().
class ChartRectItem : public ChartPixmapItem {
public:
	ChartRectItem(ChartView &v, size_t z, const QPen &pen, const QBrush &brush, double radius, bool dragable = false);
	~ChartRectItem();
	void resize(QSizeF size);
private:
	QPen pen;
	QBrush brush;
	double radius;
};

// Attention: text is only drawn after calling setColor()!
class ChartTextItem : public ChartPixmapItem {
public:
	ChartTextItem(ChartView &v, size_t z, const QFont &f, const std::vector<QString> &text, bool center);
	ChartTextItem(ChartView &v, size_t z, const QFont &f, const QString &text);
	void setColor(const QColor &color); // Draw on transparent background
	void setColor(const QColor &color, const QColor &background); // Fill rectangle with given background color
private:
	const QFont &f;
	double fontHeight;
	bool center;
	struct Item {
		QString s;
		double width;
	};
	std::vector<Item> items;
};

// Common data for line and rect items.
// Both use the "QRect rect" item of the base class.
// Lines are drawn from top-left to bottom-right.
class ChartLineItemBase : public HideableChartItem<HideableQSGNode<QSGGeometryNode>> {
public:
	ChartLineItemBase(ChartView &v, size_t z, QColor color, double width);
	~ChartLineItemBase();
	void setLine(QPointF from, QPointF to);
protected:
	QColor color;
	double width;
	bool positionDirty;
	bool materialDirty;
	std::unique_ptr<QSGFlatColorMaterial> material;
	std::unique_ptr<QSGGeometry> geometry;
};

class ChartLineItem : public ChartLineItemBase {
public:
	using ChartLineItemBase::ChartLineItemBase;
	void render() override;
};

// A simple rectangle without fill. Specified by any two opposing vertices.
class ChartRectLineItem : public ChartLineItemBase {
public:
	using ChartLineItemBase::ChartLineItemBase;
	void render() override;
};

// Implementation detail of templates - move to serparate header file
template <typename Node>
void HideableChartItem<Node>::setVisible(bool visibleIn)
{
	if (visible == visibleIn)
		return;
	visible = visibleIn;
	visibleChanged = true;
	markDirty();
}

template <typename Node>
template<class... Args>
void HideableChartItem<Node>::createNode(Args&&... args)
{
	node.reset(new Node(visible, std::forward<Args>(args)...));
	visibleChanged = false;
}

template <typename Node>
HideableChartItem<Node>::HideableChartItem(ChartView &v, size_t z, bool dragable) : ChartItem(v, z, dragable),
	visible(true), visibleChanged(false)
{
}

template <typename Node>
void HideableChartItem<Node>::updateVisible()
{
	if (visibleChanged)
		node->setVisible(visible);
	visibleChanged = false;
}

#endif
