// SPDX-License-Identifier: GPL-2.0
#include "divehandler.h"
#include "profilewidget2.h"
#include "core/gettextfromc.h"
#include "qt-models/diveplannermodel.h"
#include "qt-models/models.h"

#include <QMenu>
#include <QGraphicsSceneMouseEvent>
#include <QSettings>

DiveHandler::DiveHandler() : QGraphicsEllipseItem()
{
	setRect(-5, -5, 10, 10);
	setFlags(ItemIgnoresTransformations | ItemIsSelectable | ItemIsMovable | ItemSendsGeometryChanges);
	setBrush(Qt::white);
	setZValue(2);
	t.start();
}

int DiveHandler::parentIndex()
{
	ProfileWidget2 *view = qobject_cast<ProfileWidget2 *>(scene()->views().first());
	return view->handles.indexOf(this);
}

void DiveHandler::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
	QMenu m;
	// Don't have a gas selection for the last point
	emit released();
	DivePlannerPointsModel *plannerModel = DivePlannerPointsModel::instance();
	QModelIndex index = plannerModel->index(parentIndex(), DivePlannerPointsModel::GAS);
	if (index.sibling(index.row() + 1, index.column()).isValid()) {
		GasSelectionModel *model = GasSelectionModel::instance();
		model->repopulate();
		int rowCount = model->rowCount();
		for (int i = 0; i < rowCount; i++) {
			QAction *action = new QAction(&m);
			action->setText(model->data(model->index(i, 0), Qt::DisplayRole).toString());
			action->setData(i);
			connect(action, &QAction::triggered, this, &DiveHandler::changeGas);
			m.addAction(action);
		}
	}
	// don't allow removing the last point
	if (plannerModel->rowCount() > 1) {
		m.addSeparator();
		m.addAction(gettextFromC::tr("Remove this point"), this, &DiveHandler::selfRemove);
		m.exec(event->screenPos());
	}
}

void DiveHandler::selfRemove()
{
#ifndef SUBSURFACE_MOBILE
	setSelected(true);
	ProfileWidget2 *view = qobject_cast<ProfileWidget2 *>(scene()->views().first());
	view->keyDeleteAction();
#endif
}

void DiveHandler::changeGas()
{
	QAction *action = qobject_cast<QAction *>(sender());
	DivePlannerPointsModel *plannerModel = DivePlannerPointsModel::instance();
	QModelIndex index = plannerModel->index(parentIndex(), DivePlannerPointsModel::GAS);
	plannerModel->gasChange(index.sibling(index.row() + 1, index.column()), action->data().toInt());
}

void DiveHandler::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
	if (t.elapsed() < 40)
		return;
	t.start();

	ProfileWidget2 *view = qobject_cast<ProfileWidget2*>(scene()->views().first());
	if(view->isPointOutOfBoundaries(event->scenePos()))
		return;

	QGraphicsEllipseItem::mouseMoveEvent(event);
	emit moved();
}

void DiveHandler::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	QGraphicsItem::mousePressEvent(event);
	emit clicked();
}

void DiveHandler::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
	QGraphicsItem::mouseReleaseEvent(event);
	emit released();
}
