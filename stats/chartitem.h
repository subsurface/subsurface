// SPDX-License-Identifier: GPL-2.0
// Chart item specific to the statistics module
#ifndef STATS_CHART_ITEM_H
#define STATS_CHART_ITEM_H

#include "qt-quick/chartitem.h"

class StatsTheme;
class ChartView;

// A pie chart item: draws disk segments onto a pixmap.
class ChartPieItem : public ChartPixmapItem {
public:
	ChartPieItem(ChartView &v, size_t z, const StatsTheme &theme, double borderWidth);
	void drawSegment(double from, double to, QColor fill, QColor border, bool selected); // from and to are relative (0-1 is full disk).
	void resize(QSizeF size);	// As in base class, but clears the canvas
private:
	const StatsTheme &theme;
	double borderWidth;
};

// A bar in a bar chart: a rectangle bordered by lines.
class ChartBarItem : public HideableChartProxyItem<QSGRectangleNode> {
public:
	ChartBarItem(ChartView &v, size_t z, const StatsTheme &theme, double borderWidth);
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
	ChartBoxItem(ChartView &v, size_t z, const StatsTheme &theme, double borderWidth);
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
	ChartScatterItem(ChartView &v, size_t z, const StatsTheme &theme, bool selected);
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

#endif
