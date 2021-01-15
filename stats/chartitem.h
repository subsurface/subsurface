// SPDX-License-Identifier: GPL-2.0
// Wrappers around QSGImageNode that allow painting onto an image
// and then turning that into a texture to be displayed in a QQuickItem.
#ifndef CHART_ITEM_H
#define CHART_ITEM_H

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
	ChartItem(StatsView &v, ChartZValue z);
	virtual ~ChartItem();
	virtual void render() = 0;		// Only call on render thread!
	bool dirty;				// If true, call render() when rebuilding the scene
	ChartItem *dirtyPrev, *dirtyNext;	// Double linked list of dirty items
	const ChartZValue zValue;
protected:
	QSizeF sceneSize() const;
	StatsView &view;
};

// A chart item that blits a precalculated pixmap onto the scene.
class ChartPixmapItem : public ChartItem {
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
	std::unique_ptr<QSGImageNode> node;
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

class ChartLineItem : public ChartItem {
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
	std::unique_ptr<QSGGeometryNode> node;
	std::unique_ptr<QSGFlatColorMaterial> material;
	std::unique_ptr<QSGGeometry> geometry;
};

// A bar in a bar chart: a rectangle bordered by lines.
class ChartBarItem : public ChartItem {
public:
	ChartBarItem(StatsView &v, ChartZValue z, double borderWidth, bool horizontal);
	~ChartBarItem();
	void setColor(QColor color, QColor borderColor);
	void setRect(const QRectF &rect);
	QRectF getRect() const;
	void render() override;		// Only call on render thread!
private:
	QColor color, borderColor;
	double borderWidth;
	QRectF rect;
	bool horizontal;
	bool positionDirty;
	bool colorDirty;
	std::unique_ptr<QSGRectangleNode> node;
	std::unique_ptr<QSGGeometryNode> borderNode;
	std::unique_ptr<QSGFlatColorMaterial> borderMaterial;
	std::unique_ptr<QSGGeometry> borderGeometry;
};

#endif
