// SPDX-License-Identifier: GPL-2.0
#include "chartview.h"
#include "chartitem.h"

#include <QQuickWindow>
#include <QSGRectangleNode>

ChartView::ChartView(QQuickItem *parent, size_t maxZ) : QQuickItem(parent),
	maxZ(maxZ),
	backgroundDirty(true),
	rootNode(nullptr)
{
	setFlag(ItemHasContents, true);
}

// Define a hideable dummy QSG node that is used as a parent node to make
// all objects of a z-level visible / invisible.
using ZNode = HideableQSGNode<QSGNode>;

class RootNode : public QSGNode
{
public:
	RootNode(ChartView *view, QColor backgroundColor, size_t maxZ);
	~RootNode();
	ChartView *view;
	std::unique_ptr<QSGRectangleNode> backgroundNode; // solid background
	// We entertain one node per Z-level.
	std::vector<std::unique_ptr<ZNode>> zNodes;
};

RootNode::RootNode(ChartView *view, QColor backgroundColor, size_t maxZ) : view(view)
{
	zNodes.resize(maxZ);

	// Add a background rectangle with a solid color. This could
	// also be done on the widget level, but would have to be done
	// separately for desktop and mobile, so do it here.
	backgroundNode.reset(view->w()->createRectangleNode());
	appendChildNode(backgroundNode.get());

	for (auto &zNode: zNodes) {
		zNode.reset(new ZNode(true));
		appendChildNode(zNode.get());
	}
}

RootNode::~RootNode()
{
	if (view)
		view->emergencyShutdown();
}

ChartView::~ChartView()
{
	// Sometimes the rootNode is destructed before the view,
	// sometimes the other way around. QtQuick is a mess!
	if (rootNode)
		rootNode->view = nullptr;
}

void ChartView::freeDeletedChartItems()
{
	ChartItem *nextitem;
	for (ChartItem *item = deletedItems.first; item; item = nextitem) {
		nextitem = item->next;
		delete item;
	}
	deletedItems.clear();
}

QSGNode *ChartView::updatePaintNode(QSGNode *oldNode, QQuickItem::UpdatePaintNodeData *)
{
	// The QtQuick drawing interface is utterly bizzare with a distinct 1980ies-style memory management.
	// This is just a copy of what is found in Qt's documentation.
	RootNode *n = static_cast<RootNode *>(oldNode);
	if (!n)
		n = rootNode = new RootNode(this, backgroundColor, maxZ);

	// Delete all chart items that are marked for deletion.
	freeDeletedChartItems();

	if (backgroundDirty) {
		rootNode->backgroundNode->setRect(plotRect);
		backgroundDirty = false;
	}

	for (ChartItem *item = dirtyItems.first; item; item = item->next) {
		item->render();
		item->dirty = false;
	}
	dirtyItems.splice(cleanItems);

	return n;
}

// When reparenting the QQuickWidget, QtQuick decides to delete our rootNode
// and with it all the QSG nodes, even though we have *not* given the
// permission to do so! If the widget is reused, we try to delete the
// stale items, whose nodes have already been deleted by QtQuick, leading
// to a double-free(). Instead of searching for the cause of this behavior,
// let's just hook into the rootNode's destructor and delete the objects
// in a controlled manner, so that QtQuick has no more access to them.
void ChartView::emergencyShutdown()
{
	// Mark clean and dirty chart items for deletion...
	cleanItems.splice(deletedItems);
	dirtyItems.splice(deletedItems);
	dragableItems.clear();
	draggedItem.reset();

	// ...and delete them.
	freeDeletedChartItems();

	// Now delete all the pointers we might have to chart features,
	// axes, etc. Note that all pointers to chart items are non
	// owning, so this only resets stale references, but does not
	// lead to any additional deletion of chart items.
	resetPointers();

	// The rootNode is being deleted -> remove the reference to that
	rootNode = nullptr;
}

void ChartView::clearItems()
{
	cleanItems.splice(deletedItems);
	dirtyItems.splice(deletedItems);
}

static ZNode &getZNode(RootNode &rootNode, size_t z)
{
	size_t idx = std::clamp(z, (size_t)0, rootNode.zNodes.size());
	return *rootNode.zNodes[idx];
}

void ChartView::addQSGNode(QSGNode *node, size_t z, bool moveAfter, QSGNode *node2)
{
	auto &parent = getZNode(*rootNode, z);
	if (node2) {
		if (moveAfter)
			parent.insertChildNodeAfter(node, node2);
		else
			parent.insertChildNodeBefore(node, node2);
	} else {
		if (moveAfter)
			parent.prependChildNode(node);
		else
			parent.appendChildNode(node);
	}
}

void ChartView::moveNodeBefore(QSGNode *node, size_t z, QSGNode *before)
{
	auto &parent = getZNode(*rootNode, z);
	parent.removeChildNode(node);
	parent.insertChildNodeBefore(node, before);
}

void ChartView::moveNodeBack(QSGNode *node, size_t z)
{
	auto &parent = getZNode(*rootNode, z);
	parent.removeChildNode(node);
	parent.appendChildNode(node);
}

void ChartView::moveNodeAfter(QSGNode *node, size_t z, QSGNode *before)
{
	auto &parent = getZNode(*rootNode, z);
	parent.removeChildNode(node);
	parent.insertChildNodeAfter(node, before);
}

void ChartView::moveNodeFront(QSGNode *node, size_t z)
{
	auto &parent = getZNode(*rootNode, z);
	parent.removeChildNode(node);
	parent.prependChildNode(node);
}

void ChartView::registerChartItem(ChartItem &item)
{
	cleanItems.append(item);
	if (item.dragable)
		dragableItems.push_back(&item);
}

void ChartView::registerDirtyChartItem(ChartItem &item)
{
	if (item.dirty)
		return;
	cleanItems.remove(item);
	dirtyItems.append(item);
	item.dirty = true;
}

void ChartView::deleteChartItemInternal(ChartItem &item)
{
	if (item.dirty)
		dirtyItems.remove(item);
	else
		cleanItems.remove(item);
	deletedItems.append(item);
	if (item.dragable) {
		// This becomes inefficient if there are many dragable items!
		auto it = std::find(dragableItems.begin(), dragableItems.end(), &item);
		if (it == dragableItems.end())
			fprintf(stderr, "warning: dragable item not found\n");
		else
			dragableItems.erase(it);
	}
}

ChartView::ChartItemList::ChartItemList() : first(nullptr), last(nullptr)
{
}

void ChartView::ChartItemList::clear()
{
	first = last = nullptr;
}

void ChartView::ChartItemList::remove(ChartItem &item)
{
	if (item.next)
		item.next->prev = item.prev;
	else
		last = item.prev;
	if (item.prev)
		item.prev->next = item.next;
	else
		first = item.next;
	item.prev = item.next = nullptr;
}

void ChartView::ChartItemList::append(ChartItem &item)
{
	if (!first) {
		item.prev = nullptr;
		first = &item;
	} else {
		item.prev = last;
		last->next = &item;
	}
	item.next = nullptr;
	last = &item;
}

void ChartView::ChartItemList::splice(ChartItemList &l2)
{
	if (!first) // if list is empty -> nothing to do.
		return;
	if (!l2.first) {
		l2 = *this;
	} else {
		l2.last->next = first;
		first->prev = l2.last;
		l2.last = last;
	}
	clear();
}

QQuickWindow *ChartView::w() const
{
	return window();
}

void ChartView::setBackgroundColor(QColor color)
{
	backgroundColor = color;
	if (rootNode)
		rootNode->backgroundNode->setColor(color);
}

QSizeF ChartView::size() const
{
	return boundingRect().size();
}

QRectF ChartView::plotArea() const
{
	return plotRect;
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
void ChartView::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
#else
void ChartView::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
#endif
{
	plotRect = QRectF(QPointF(0.0, 0.0), newGeometry.size());
	backgroundDirty = true;
	plotAreaChanged(plotRect.size());

	// Do we need to call the base-class' version of geometryChanged? Probably for QML?
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	QQuickItem::geometryChange(newGeometry, oldGeometry);
#else
	QQuickItem::geometryChanged(newGeometry, oldGeometry);
#endif
}

void ChartView::setLayerVisibility(size_t z, bool visible)
{
	if (rootNode && z < rootNode->zNodes.size())
		rootNode->zNodes[z]->setVisible(visible);
}

void ChartView::mousePressEvent(QMouseEvent *event)
{
	QPointF pos = event->localPos();

	for (auto item: dragableItems) {
		QRectF rect = item->getRect();
		if (rect.contains(pos)) {
			dragStartMouse = pos;
			dragStartItem = rect.topLeft();
			draggedItem = item;
			grabMouse();
			setKeepMouseGrab(true); // don't allow Qt to steal the grab
			event->accept();
			return;
		}
	}

	event->ignore();
}

void ChartView::mouseReleaseEvent(QMouseEvent *event)
{
	if (draggedItem) {
		QPointF pos = event->localPos();
		draggedItem->stopDrag(pos);
		draggedItem.reset();
		ungrabMouse();
		event->accept();
	}
}

void ChartView::mouseMoveEvent(QMouseEvent *event)
{
	if (draggedItem) {
		QSizeF sceneSize = size();
		if (sceneSize.width() <= 1.0 || sceneSize.height() <= 1.0)
			return;
		draggedItem->setPos(event->pos() - dragStartMouse + dragStartItem);
		update();
	}
}
