// SPDX-License-Identifier: GPL-2.0
#include "handleitem.h"
#include "profileview.h"
#include "zvalues.h"

#include <QApplication>

static QColor handleBorderColor(Qt::black);
static QColor handleColor(Qt::white);
static QColor gasColor(Qt::black);
static constexpr double handleRadius = 5.0;

class HandleItemHandle : public ChartDiskItem {
	ProfileView &profileView;
	int idx;
public:
	HandleItemHandle(ChartView &view, double dpr, int idx, ProfileView &profileView) :
		ChartDiskItem(view,
			      ProfileZValue::Handles,
			      QPen(handleBorderColor, dpr),
			      QBrush(handleColor),
			      true),
		profileView(profileView),
		idx(idx)
	{
	}
	void setIdx(int idxIn)
	{
		idx = idxIn;
	}
	void drag(QPointF pos) override
	{
		profileView.handleDragged(idx, pos);
	}
	void startDrag(QPointF) override
	{
		profileView.handleSelected(idx);
	}
	void stopDrag(QPointF) override
	{
		profileView.handleReleased(idx);
	}
};

HandleItem::HandleItem(ProfileView &view, double dpr, int idx) :
		handle(view.createChartItem<HandleItemHandle>(dpr, idx, view)),
		dpr(dpr),
		view(view)
{
	handle->resize(handleRadius * dpr);
}

HandleItem::~HandleItem()
{
}

void HandleItem::del()
{
	handle.del();
	if (text)
		text.del();
}

void HandleItem::setIdx(int idx)
{
	handle->setIdx(idx);
}

void HandleItem::setPos(QPointF pos)
{
	handle->setPos(pos);
}

QPointF HandleItem::getPos() const
{
	return handle->getPos();
}

void HandleItem::setTextPos(QPointF pos)
{
	if (text)
		text->setPos(pos);
}

void HandleItem::setVisible(bool handleVisible, bool textVisible)
{
	handle->setVisible(handleVisible);
	if (text)
		text->setVisible(textVisible);
}

// duplicate code in tooltipitem.cpp
static QFont makeFont(double dpr)
{
	QFont font(qApp->font());
	if (dpr != 1.0) {
		int pixelSize = font.pixelSize();
		if (pixelSize > 0) {
			pixelSize = lrint(static_cast<double>(pixelSize) * dpr);
			font.setPixelSize(pixelSize);
		} else {
			font.setPointSizeF(font.pointSizeF() * dpr);
		}
	}
	return font;
}

void HandleItem::setText(const QString &s)
{
	if (text && std::exchange(oldText, s) == s)
		return;
	if (text)
		text.del();

	QFont f = makeFont(dpr);
	text = view.createChartItem<ChartTextItem>(ProfileZValue::Handles, f, s);
	text->setColor(gasColor);
}

/*
void HandleItem::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
	QMenu m;
	// Don't have a gas selection for the last point
	emit released();
	DivePlannerPointsModel *plannerModel = DivePlannerPointsModel::instance();
	QModelIndex index = plannerModel->index(parentIndex(), DivePlannerPointsModel::GAS);
	if (index.sibling(index.row() + 1, index.column()).isValid()) {
		QStringList gases = get_dive_gas_list(dive);
		for (int i = 0; i < gases.size(); i++) {
			QAction *action = new QAction(&m);
			action->setText(gases[i]);
			action->setData(i);
			connect(action, &QAction::triggered, this, &HandleItem::changeGas);
			m.addAction(action);
		}
	}
	// don't allow removing the last point
	if (plannerModel->rowCount() > 1) {
		m.addSeparator();
		m.addAction(gettextFromC::tr("Remove this point"), this, &HandleItem::selfRemove);
		m.exec(event->screenPos());
	}
}

void HandleItem::changeGas()
{
	QAction *action = qobject_cast<QAction *>(sender());
	DivePlannerPointsModel *plannerModel = DivePlannerPointsModel::instance();
	QModelIndex index = plannerModel->index(parentIndex(), DivePlannerPointsModel::GAS);
	plannerModel->gasChange(index.sibling(index.row() + 1, index.column()), action->data().toInt());
}
*/
