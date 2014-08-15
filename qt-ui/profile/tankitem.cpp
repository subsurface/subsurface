#include "tankitem.h"
#include "diveplotdatamodel.h"
#include "divetextitem.h"
#include "profile.h"
#include <QGradient>
#include <QDebug>

TankItem::TankItem(QObject *parent) :
	QGraphicsRectItem(),
	dataModel(0),
	dive(0),
	pInfo(0)
{
	yPos = 92;
	height = 3;
	QColor red(PERSIANRED1);
	QColor blue(AIR_BLUE);
	QColor yellow(NITROX_YELLOW);
	QColor green(NITROX_GREEN);
	QLinearGradient nitroxGradient(QPointF(0, yPos), QPointF(0, yPos + height));
	nitroxGradient.setColorAt(0.0, green);
	nitroxGradient.setColorAt(0.49, green);
	nitroxGradient.setColorAt(0.5, yellow);
	nitroxGradient.setColorAt(1.0, yellow);
	nitrox = nitroxGradient;
	QLinearGradient trimixGradient(QPointF(0, yPos), QPointF(0, yPos + height));
	trimixGradient.setColorAt(0.0, green);
	trimixGradient.setColorAt(0.49, green);
	trimixGradient.setColorAt(0.5, red);
	trimixGradient.setColorAt(1.0, red);
	trimix = trimixGradient;
	air = blue;
}

void TankItem::setData(DivePlotDataModel *model, struct plot_info *plotInfo, struct dive *d)
{
	pInfo = plotInfo;
	dive = d;
	dataModel = model;
	connect(dataModel, SIGNAL(dataChanged(QModelIndex, QModelIndex)), this, SLOT(modelDataChanged(QModelIndex, QModelIndex)));
	modelDataChanged();
}

void TankItem::createBar(qreal x, qreal w, struct gasmix *gas)
{
	// pick the right gradient, size, position and text
	QGraphicsRectItem *rect = new QGraphicsRectItem(x, yPos, w, height, this);
	if (gasmix_is_air(gas))
		rect->setBrush(air);
	else if (gas->he.permille)
		rect->setBrush(trimix);
	else
		rect->setBrush(nitrox);
	rects.push_back(rect);
	DiveTextItem *label = new DiveTextItem(rect);
	label->setText(gasname(gas));
	label->setBrush(Qt::black);
	label->setPos(x, yPos);
	label->setAlignment(Qt::AlignBottom | Qt::AlignRight);
	label->setZValue(101);
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

	// walk the list and figure out which tanks go where
	struct plot_data *entry = pInfo->entry;
	int cylIdx = entry->cylinderindex;
	int i = -1;
	int startTime = 0;
	struct gasmix *gas = &dive->cylinder[cylIdx].gasmix;
	qreal width, left;
	while (++i < pInfo->nr) {
		entry = &pInfo->entry[i];
		if (entry->cylinderindex == cylIdx)
			continue;
		width = hAxis->posAtValue(entry->sec) - hAxis->posAtValue(startTime);
		left = hAxis->posAtValue(startTime);
		createBar(left, width, gas);
		cylIdx = entry->cylinderindex;
		gas = &dive->cylinder[cylIdx].gasmix;
		startTime = entry->sec;
	}
	width = hAxis->posAtValue(entry->sec) - hAxis->posAtValue(startTime);
	left = hAxis->posAtValue(startTime);
	createBar(left, width, gas);
}

void TankItem::setHorizontalAxis(DiveCartesianAxis *horizontal)
{
	hAxis = horizontal;
	connect(hAxis, SIGNAL(sizeChanged()), this, SLOT(modelDataChanged()));
	modelDataChanged();
}
