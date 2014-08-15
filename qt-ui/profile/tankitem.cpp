#include "tankitem.h"
#include "diveplotdatamodel.h"
#include "profile.h"
#include <QGradient>
#include <QDebug>

TankItem::TankItem(QObject *parent) :
	QGraphicsRectItem(),
	dataModel(0),
	dive(0),
	pInfo(0)
{
}

void TankItem::setData(DivePlotDataModel *model, struct plot_info *plotInfo, struct dive *d)
{
	pInfo = plotInfo;
	dive = d;
	dataModel = model;
	connect(dataModel, SIGNAL(dataChanged(QModelIndex, QModelIndex)), this, SLOT(modelDataChanged(QModelIndex, QModelIndex)));
	modelDataChanged();
}

void TankItem::modelDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
	// We don't have enougth data to calculate things, quit.

	if (!dive || !dataModel || !pInfo || !pInfo->nr)
		return;

	// remove the old rectangles
	foreach (QGraphicsRectItem *r, rects) {
		delete(r);
	}
	rects.clear();

	// define the position on the profile
	qreal width, left, yPos, height;
	yPos = 95.0;
	height = 3.0;

	// set up the three patterns
	QLinearGradient nitrox(QPointF(0, yPos), QPointF(0, yPos + height));
	nitrox.setColorAt(0.0, Qt::green);
	nitrox.setColorAt(0.49, Qt::green);
	nitrox.setColorAt(0.5, Qt::yellow);
	nitrox.setColorAt(1.0, Qt::yellow);
	QLinearGradient trimix(QPointF(0, yPos), QPointF(0, yPos + height));
	trimix.setColorAt(0.0, Qt::green);
	trimix.setColorAt(0.49, Qt::green);
	trimix.setColorAt(0.5, Qt::red);
	trimix.setColorAt(1.0, Qt::red);
	QColor air(Qt::blue);
	air.lighter();

	// walk the list and figure out which tanks go where
	struct plot_data *entry = pInfo->entry;
	int cylIdx = entry->cylinderindex;
	int i = -1;
	int startTime = 0;
	struct gasmix *gas = &dive->cylinder[cylIdx].gasmix;
	while (++i < pInfo->nr) {
		entry = &pInfo->entry[i];
		if (entry->cylinderindex == cylIdx)
			continue;
		width = hAxis->posAtValue(entry->sec) - hAxis->posAtValue(startTime);
		left = hAxis->posAtValue(startTime);
		QGraphicsRectItem *rect = new QGraphicsRectItem(left, yPos, width, height, this);
		if (gasmix_is_air(gas))
			rect->setBrush(air);
		else if (gas->he.permille)
			rect->setBrush(trimix);
		else
			rect->setBrush(nitrox);
		rects << rect;
		cylIdx = entry->cylinderindex;
		gas = &dive->cylinder[cylIdx].gasmix;
		startTime = entry->sec;
	}
	width = hAxis->posAtValue(entry->sec) - hAxis->posAtValue(startTime);
	left = hAxis->posAtValue(startTime);
	QGraphicsRectItem *rect = new QGraphicsRectItem(left, yPos, width, height, this);
	if (gasmix_is_air(gas))
		rect->setBrush(air);
	else if (gas->he.permille)
		rect->setBrush(trimix);
	else
		rect->setBrush(nitrox);
	rects << rect;
}

void TankItem::setHorizontalAxis(DiveCartesianAxis *horizontal)
{
	hAxis = horizontal;
	connect(hAxis, SIGNAL(sizeChanged()), this, SLOT(modelDataChanged()));
	modelDataChanged();
}
