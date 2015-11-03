#ifndef TANKITEM_H
#define TANKITEM_H

#include <QGraphicsItem>
#include <QModelIndex>
#include <QBrush>
#include "divelineitem.h"
#include "divecartesianaxis.h"
#include "dive.h"

class TankItem : public QObject, public QGraphicsRectItem
{
	Q_OBJECT

public:
	explicit TankItem(QObject *parent = 0);
	~TankItem();
	void setHorizontalAxis(DiveCartesianAxis *horizontal);
	void setData(DivePlotDataModel *model, struct plot_info *plotInfo, struct dive *d);

signals:

public slots:
	virtual void modelDataChanged(const QModelIndex &topLeft = QModelIndex(), const QModelIndex &bottomRight = QModelIndex());

private:
	void createBar(qreal x, qreal w, struct gasmix *gas);
	DivePlotDataModel *dataModel;
	DiveCartesianAxis *hAxis;
	int hDataColumn;
	struct dive diveCylinderStore;
	struct plot_data *pInfoEntry;
	int pInfoNr;
	qreal height;
	QBrush air, nitrox, oxygen, trimix;
	QList<QGraphicsRectItem *> rects;
};

#endif // TANKITEM_H
