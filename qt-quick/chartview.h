// SPDX-License-Identifier: GPL-2.0
#ifndef CHART_VIEW_H
#define CHART_VIEW_H

#include "chartitem_ptr.h"

#include <QQuickItem>

class ChartItem;
class QSGTexture;
class RootNode;	// Internal implementation detail

class ChartView : public QQuickItem {
	Q_OBJECT
public:
	ChartView(QQuickItem *parent, size_t maxZ);
	~ChartView();

	QQuickWindow *w() const;			// Make window available to items
	QSizeF size() const;
	QRectF plotArea() const;
	void setBackgroundColor(QColor color);		// Chart must be replot for color to become effective.
	void addQSGNode(QSGNode *node, size_t z, bool moveAfter, QSGNode *node2);
							// Must only be called in render thread!
							// If node2 is nullptr move to begin or end of list.
	void moveNodeBefore(QSGNode *node, size_t z, QSGNode *before);
	void moveNodeBack(QSGNode *node, size_t z);
	void moveNodeAfter(QSGNode *node, size_t z, QSGNode *after);
	void moveNodeFront(QSGNode *node, size_t z);
	void registerChartItem(ChartItem &item);
	void registerDirtyChartItem(ChartItem &item);
	void emergencyShutdown();			// Called when QQuick decides to delete our root node.

	// Create a chart item and add it to the scene.
	// The item must not be deleted by the caller, but can be
	// scheduled for deletion using deleteChartItem() below.
	// Most items can be made invisible, which is preferred over deletion.
	// All items on the scene will be deleted once the chart is reset.
	template <typename T, class... Args>
	ChartItemPtr<T> createChartItem(Args&&... args);

	template <typename T>
	void deleteChartItem(ChartItemPtr<T> &item);

protected:
	void setLayerVisibility(size_t z, bool visible);
	void clearItems();

	// This is called when Qt decided to reset our rootNode, which invalidates all items on the chart.
	// The base class must invalidate all pointers and references.
	virtual void resetPointers() = 0;

	void mousePressEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
private:
	// QtQuick related things
	size_t maxZ;
	bool backgroundDirty;
	QRectF plotRect;
	QSGNode *updatePaintNode(QSGNode *oldNode, QQuickItem::UpdatePaintNodeData *updatePaintNodeData) override;
	QColor backgroundColor;

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
#else
	void geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry) override;
#endif
	virtual void plotAreaChanged(const QSizeF &size) = 0;

	RootNode *rootNode;

	// There are three double linked lists of chart items:
	// clean items, dirty items and items to be deleted.
	// Note that only the render thread must delete chart items,
	// and therefore these lists are the only owning pointers
	// to chart items. All other pointers are non-owning and
	// can therefore become stale.
	struct ChartItemList {
		ChartItemList();
		ChartItem *first, *last;
		void append(ChartItem &item);
		void remove(ChartItem &item);
		void clear();
		void splice(ChartItemList &list);
	};
	ChartItemList cleanItems, dirtyItems, deletedItems;
	void deleteChartItemInternal(ChartItem &item);
	void freeDeletedChartItems();

	// Keep a list of dragable items. For now, there are no many,
	// so keep it unsorted. In the future we might want to sort by
	// coordinates.
	std::vector<ChartItem *> dragableItems;
	ChartItemPtr<ChartItem> draggedItem;
	QPointF dragStartMouse, dragStartItem;
};

// This implementation detail must be known to users of the class.
// Perhaps move it into a chartview_impl.h file.
template <typename T, class... Args>
ChartItemPtr<T> ChartView::createChartItem(Args&&... args)
{
	return ChartItemPtr(new T(*this, std::forward<Args>(args)...));
}

template <typename T>
void ChartView::deleteChartItem(ChartItemPtr<T> &item)
{
	deleteChartItemInternal(*item);
	item.reset();
}

#endif
