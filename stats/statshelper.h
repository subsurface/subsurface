// SPDX-License-Identifier: GPL-2.0
// Helper functions to render the stats. Currently only
// contains a small template to create scene-items. This
// is for historical reasons to ease transition from QtCharts
// and might be removed.
#ifndef STATSHELPER_H

#include <memory>
#include <QGraphicsScene>

template <typename T, class... Args>
T *createItem(QGraphicsScene *scene, Args&&... args)
{
	T *res = new T(std::forward<Args>(args)...);
	scene->addItem(res);
	return res;
}

template <typename T, class... Args>
std::unique_ptr<T> createItemPtr(QGraphicsScene *scene, Args&&... args)
{
	return std::unique_ptr<T>(createItem<T>(scene, std::forward<Args>(args)...));
}

#endif
