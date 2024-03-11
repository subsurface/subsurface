// SPDX-License-Identifier: GPL-2.0
// Private template implementation for ChartItem child classes

#ifndef CHARTITEM_PRIVATE_H
#define CHARTITEM_PRIVATE_H

#include "chartitem.h"
#include "chartview.h"

template <typename Node>
template<class... Args>
void HideableChartItem<Node>::createNode(Args&&... args)
{
	node.reset(new Node(visible, std::forward<Args>(args)...));
	visibleChanged = false;
}

template <typename Node>
void HideableChartItem<Node>::addNodeToView()
{
	view.addQSGNode(node.get(), zValue, moveMode == MoveMode::after, moveNode);
	moveNode = nullptr;
	moveMode = MoveMode::none;
}

template <typename Node>
HideableChartItem<Node>::HideableChartItem(ChartView &v, size_t z, bool dragable) : ChartItem(v, z, dragable),
	visible(true), visibleChanged(false), moveMode(MoveMode::none), moveNode(nullptr)
{
}

template <typename Node>
void HideableChartItem<Node>::updateVisible()
{
	if (visibleChanged)
		node->setVisible(visible);
	visibleChanged = false;
}

template <typename Node>
void HideableChartItem<Node>::doRearrange()
{
	if (!node)
		return;
	switch (moveMode) {
	default:
	case MoveMode::none:
		return;
	case MoveMode::before:
		if (moveNode)
			view.moveNodeBefore(node.get(), zValue, moveNode);
		else
			view.moveNodeBack(node.get(), zValue);
		break;
	case MoveMode::after:
		if (moveNode)
			view.moveNodeAfter(node.get(), zValue, moveNode);
		else
			view.moveNodeFront(node.get(), zValue);
		break;
	}
	moveNode = nullptr;
	moveMode = MoveMode::none;
}

#endif
