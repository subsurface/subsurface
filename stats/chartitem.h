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
class QSGTextureMaterial;
class StatsTheme;
class StatsView;
enum class ChartZValue : int;

class ChartItem {
public:
	// Only call on render thread!
	virtual void render() = 0;
	bool dirty;				// If true, call render() when rebuilding the scene
	ChartItem *prev, *next;			// Double linked list of items
	const ChartZValue zValue;
	virtual ~ChartItem();			// Attention: must only be called by render thread.
protected:
	ChartItem(StatsView &v, ChartZValue z);
	QSizeF sceneSize() const;
	StatsView &view;
	void markDirty();
};

template <typename Node>
class HideableChartItem : public ChartItem {
protected:
	HideableChartItem(StatsView &v, ChartZValue z);
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
	ChartPixmapItem(StatsView &v, ChartZValue z);
	~ChartPixmapItem();

	void setPos(QPointF pos);
	void render() override;
	QRectF getRect() const;
protected:
	void resize(QSizeF size);	// Resets the canvas. Attention: image is *unitialized*.
	std::unique_ptr<QPainter> painter;
	std::unique_ptr<QImage> img;
	void setTextureDirty();
	void setPositionDirty();
	QRectF rect;
private:
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
	ChartTextItem(StatsView &v, ChartZValue z, const QFont &f, const QString &text);
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

// A pie chart item: draws disk segments onto a pixmap.
class ChartPieItem : public ChartPixmapItem {
public:
	ChartPieItem(StatsView &v, ChartZValue z, const StatsTheme &theme, double borderWidth);
	void drawSegment(double from, double to, QColor fill, QColor border, bool selected); // from and to are relative (0-1 is full disk).
	void resize(QSizeF size);	// As in base class, but clears the canvas
private:
	const StatsTheme &theme;
	double borderWidth;
};

// Common data for line and rect items. Both are represented by two points.
class ChartLineItemBase : public HideableChartItem<HideableQSGNode<QSGGeometryNode>> {
public:
	ChartLineItemBase(StatsView &v, ChartZValue z, QColor color, double width);
	~ChartLineItemBase();
	void setLine(QPointF from, QPointF to);
protected:
	QPointF from, to;
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

// A bar in a bar chart: a rectangle bordered by lines.
class ChartBarItem : public HideableChartProxyItem<QSGRectangleNode> {
public:
	ChartBarItem(StatsView &v, ChartZValue z, const StatsTheme &theme, double borderWidth);
	~ChartBarItem();
	void setColor(QColor color, QColor borderColor);
	void setRect(const QRectF &rect);
	void setSelected(bool selected);
	QRectF getRect() const;
	void render() override;
protected:
	const StatsTheme &theme;
	QColor color, borderColor;
	double borderWidth;
	QRectF rect;
	bool selected;
	bool positionDirty;
	bool colorDirty;
	bool selectedDirty;
	std::unique_ptr<QSGGeometryNode> borderNode;
	std::unique_ptr<QSGFlatColorMaterial> borderMaterial;
	std::unique_ptr<QSGGeometry> borderGeometry;
private:
	// Overlay for selected items. Created on demand.
	std::unique_ptr<QSGGeometryNode> selectionNode;
	std::unique_ptr<QSGTextureMaterial> selectionMaterial;
	std::unique_ptr<QSGGeometry> selectionGeometry;
	QSGTexture *getSelectedTexture() const;
};

// A box-and-whiskers item. This is a bit lazy: derive from the bar item and add whiskers.
class ChartBoxItem : public ChartBarItem {
public:
	ChartBoxItem(StatsView &v, ChartZValue z, const StatsTheme &theme, double borderWidth);
	~ChartBoxItem();
	void setBox(const QRectF &rect, double min, double max, double median); // The rect describes Q1, Q3.
	QRectF getRect() const;		// Note: this extends the center rectangle to include the whiskers.
	void render() override;
private:
	double min, max, median;
	std::unique_ptr<QSGGeometryNode> whiskersNode;
	std::unique_ptr<QSGFlatColorMaterial> whiskersMaterial;
	std::unique_ptr<QSGGeometry> whiskersGeometry;
};

// An item in a scatter chart. This is not simply a normal pixmap item,
// because we want that all items share the *same* texture for memory
// efficiency. It is somewhat questionable to define the form of the
// scatter item here, but so it is for now.
class ChartScatterItem : public HideableChartProxyItem<QSGImageNode> {
public:
	ChartScatterItem(StatsView &v, ChartZValue z, const StatsTheme &theme, bool selected);
	~ChartScatterItem();

	// Currently, there is no highlighted and selected status.
	enum class Highlight {
		Unselected,
		Selected,
		Highlighted
	};
	void setPos(QPointF pos);		// Specifies the *center* of the item.
	void setHighlight(Highlight highlight);	// In the future, support different kinds of scatter items.
	void render() override;
	QRectF getRect() const;
	bool contains(QPointF point) const;
	bool inRect(const QRectF &rect) const;
private:
	const StatsTheme &theme;
	QSGTexture *getTexture() const;
	QRectF rect;
	QSizeF textureSize;
	bool positionDirty, textureDirty;
	Highlight highlight;
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
HideableChartItem<Node>::HideableChartItem(StatsView &v, ChartZValue z) : ChartItem(v, z),
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
