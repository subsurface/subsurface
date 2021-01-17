// SPDX-License-Identifier: GPL-2.0
// Wrappers around QSGImageNode that allow painting onto an image
// and then turning that into a texture to be displayed in a QQuickItem.
#ifndef CHART_ITEM_H
#define CHART_ITEM_H

#include "statshelper.h"

#include <memory>
#include <QPainter>

class QSGGeometry;
class QSGGeometryNode;
class QSGFlatColorMaterial;
class QSGImageNode;
class QSGRectangleNode;
class QSGTexture;
class StatsView;
enum class ChartZValue : int;

class ChartItem {
public:
	virtual void render() = 0;		// Only call on render thread!
	bool dirty;				// If true, call render() when rebuilding the scene
	ChartItem *dirtyPrev, *dirtyNext;	// Double linked list of dirty items
	const ChartZValue zValue;
protected:
	ChartItem(StatsView &v, ChartZValue z);
	virtual ~ChartItem();
	QSizeF sceneSize() const;
	StatsView &view;
};

template <typename Node>
class HideableChartItem : public ChartItem {
protected:
	HideableChartItem(StatsView &v, ChartZValue z);
	std::unique_ptr<Node> node;
	bool visible;				// Argh. If visibility is set before node is created, we have to cache it.
	template<class... Args>
	void createNode(Args&&... args);	// Call to create node with visibility flag.
public:
	void setVisible(bool visible);
};

// A shortcut for ChartItems based on a hideable proxy item
template <typename Node>
using HideableChartProxyItem = HideableChartItem<HideableQSGNode<QSGProxyNode<Node>>>;

// A chart item that blits a precalculated pixmap onto the scene.
class ChartPixmapItem : public HideableChartProxyItem<QSGImageNode> {
public:
	ChartPixmapItem(StatsView &v, ChartZValue z);
	~ChartPixmapItem();

	void setPos(QPointF pos);
	void render() override;		// Only call on render thread!
	QRectF getRect() const;
protected:
	void resize(QSizeF size);	// Resets the canvas. Attention: image is *unitialized*.
	std::unique_ptr<QPainter> painter;
	std::unique_ptr<QImage> img;
	void setTextureDirty();
	void setPositionDirty();
private:
	QRectF rect;
	bool positionDirty;		// true if the position changed since last render
	bool textureDirty;		// true if the pixmap changed since last render
	std::unique_ptr<QSGTexture> texture;
};

// Draw a rectangular background after resize. Children are responsible for calling update().
class ChartRectItem : public ChartPixmapItem {
public:
	ChartRectItem(StatsView &v, ChartZValue z, const QPen &pen, const QBrush &brush, double radius);
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
	ChartTextItem(StatsView &v, ChartZValue z, const QFont &f, const std::vector<QString> &text, bool center);
	void setColor(const QColor &color);
private:
	QFont f;
	double fontHeight;
	bool center;
	struct Item {
		QString s;
		double width;
	};
	std::vector<Item> items;
};

class ChartLineItem : public HideableChartItem<HideableQSGNode<QSGGeometryNode>> {
public:
	ChartLineItem(StatsView &v, ChartZValue z, QColor color, double width);
	~ChartLineItem();
	void setLine(QPointF from, QPointF to);
	void render() override;		// Only call on render thread!
private:
	QPointF from, to;
	QColor color;
	double width;
	bool horizontal;
	bool positionDirty;
	bool materialDirty;
	std::unique_ptr<QSGFlatColorMaterial> material;
	std::unique_ptr<QSGGeometry> geometry;
};

// A bar in a bar chart: a rectangle bordered by lines.
class ChartBarItem : public HideableChartProxyItem<QSGRectangleNode> {
public:
	ChartBarItem(StatsView &v, ChartZValue z, double borderWidth, bool horizontal);
	~ChartBarItem();
	void setColor(QColor color, QColor borderColor);
	void setRect(const QRectF &rect);
	QRectF getRect() const;
	void render() override;		// Only call on render thread!
protected:
	QColor color, borderColor;
	double borderWidth;
	QRectF rect;
	bool horizontal;
	bool positionDirty;
	bool colorDirty;
	std::unique_ptr<QSGGeometryNode> borderNode;
	std::unique_ptr<QSGFlatColorMaterial> borderMaterial;
	std::unique_ptr<QSGGeometry> borderGeometry;
};

// A box-and-whiskers item. This is a bit lazy: derive from the bar item and add whiskers.
class ChartBoxItem : public ChartBarItem {
public:
	ChartBoxItem(StatsView &v, ChartZValue z, double borderWidth);
	~ChartBoxItem();
	void setBox(const QRectF &rect, double min, double max, double median); // The rect describes Q1, Q3.
	QRectF getRect() const;		// Note: this extends the center rectangle to include the whiskers.
	void render() override;		// Only call on render thread!
private:
	double min, max, median;
	std::unique_ptr<QSGGeometryNode> whiskersNode;
	std::unique_ptr<QSGFlatColorMaterial> whiskersMaterial;
	std::unique_ptr<QSGGeometry> whiskersGeometry;
};


// Implementation detail of templates - move to serparate header file
template <typename Node>
void HideableChartItem<Node>::setVisible(bool visibleIn)
{
	visible = visibleIn;
	if (node)
		node->setVisible(visible);
}

template <typename Node>
template<class... Args>
void HideableChartItem<Node>::createNode(Args&&... args)
{
	node.reset(new Node(visible, std::forward<Args>(args)...));
}

template <typename Node>
HideableChartItem<Node>::HideableChartItem(StatsView &v, ChartZValue z) : ChartItem(v, z),
	visible(true)
{
}

#endif
