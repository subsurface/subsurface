// SPDX-License-Identifier: GPL-2.0
#include "divehandler.h"
#include "profilewidget2.h"
#include "profilescene.h"
#include "core/dive.h"
#include "core/gettextfromc.h"
#include "core/qthelper.h"
#include "qt-models/diveplannermodel.h"

#include <QMenu>
#include <QGraphicsSceneMouseEvent>
#include <QSettings>

DiveHandler::DiveHandler(const struct dive *d, int currentDcNr) : dive(d), dcNr(currentDcNr)
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
	return view->handleIndex(this);
}

void DiveHandler::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
	QMenu m;
	// Don't have a gas selection for the last point
	emit released();

	DivePlannerPointsModel *plannerModel = DivePlannerPointsModel::instance();
	QModelIndex index = plannerModel->index(parentIndex(), DivePlannerPointsModel::GAS);
	if (index.sibling(index.row() + 1, index.column()).isValid()) {
		std::vector<std::pair<int, QString>> gases = get_dive_gas_list(dive, dcNr, true);
		for (unsigned i = 0; i < gases.size(); i++) {
			QAction *action = new QAction(&m);
			action->setText(gases[i].second);
			action->setData(gases[i].first);
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
	ProfileWidget2 *view = qobject_cast<ProfileWidget2 *>(scene()->views().first());
	QAction *action = qobject_cast<QAction *>(sender());

	view->changeGas(parentIndex(), action->data().toInt());
}

void DiveHandler::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
	if (t.elapsed() < 40)
		return;
	t.start();

	ProfileWidget2 *view = qobject_cast<ProfileWidget2*>(scene()->views().first());
	if(!view->profileScene->pointOnProfile(event->scenePos()))
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
