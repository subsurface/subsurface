// SPDX-License-Identifier: GPL-2.0
#ifndef DIVEHANDLER_HPP
#define DIVEHANDLER_HPP

#include "qt-quick/chartitem.h"

class HandleItemHandle;
class HandleItemText;
class ProfileView;

class HandleItem {
	ChartItemPtr<HandleItemHandle> handle;
	ChartItemPtr<ChartTextItem> text;
	QString oldText;
public:
	HandleItem(ProfileView &view, double dpr, int idx);
	~HandleItem();
	void setVisible(bool handle, bool text);
	void setPos(QPointF point);
	QPointF getPos() const;
	void setTextPos(QPointF point);
	void setText(const QString &text);
	void setIdx(int idx); // we may have to rearrange the handles when editing the dive
	void del(); // Deletes objects - must not be used any longer
private:
	double dpr;
	ProfileView &view;
protected:
	//void contextMenuEvent(QGraphicsSceneContextMenuEvent *event);
	//void mousePressEvent(QGraphicsSceneMouseEvent *event);
	//void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
public
slots:
	//void selfRemove();
	//void changeGas();
};

#endif
