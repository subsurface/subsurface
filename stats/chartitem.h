// SPDX-License-Identifier: GPL-2.0
// Wrappers around QSGImageNode that allow painting onto an image
// and then turning that into a texture to be displayed in a QQuickItem.
#ifndef CHART_ITEM_H
#define CHART_ITEM_H

#include <memory>
#include <QPainter>

class QSGImageNode;
class QSGTexture;
class StatsView;

class ChartItem {
public:
	ChartItem(StatsView &v);
	~ChartItem();
	// Attention: The children are responsible for updating the item. None of these calls will.
	void resize(QSizeF size);	// Resets the canvas. Attention: image is *unitialized*.
	void setPos(QPointF pos);
	void render();			// Only call on render thread!
	QRectF getRect() const;
	bool dirty;			// If true, call render() when rebuilding the scene
protected:
	std::unique_ptr<QPainter> painter;
	std::unique_ptr<QImage> img;
	QSizeF sceneSize() const;
	void setTextureDirty();
	void setPositionDirty();
private:
	StatsView &view;
	QRectF rect;
	bool positionDirty;
	bool textureDirty;
	std::unique_ptr<QSGImageNode> node;
	std::unique_ptr<QSGTexture> texture;
};

// Draw a rectangular background after resize. Children are responsible for calling update().
class ChartRectItem : public ChartItem {
public:
	ChartRectItem(StatsView &v, const QPen &pen, const QBrush &brush, double radius);
	~ChartRectItem();
	void resize(QSizeF size);
private:
	QPen pen;
	QBrush brush;
	double radius;
};

#endif
