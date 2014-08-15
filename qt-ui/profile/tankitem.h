#ifndef TANKITEM_H
#define TANKITEM_H

#include <QGraphicsItem>
#include <QModelIndex>
#include "divelineitem.h"
#include "divecartesianaxis.h"
#include "dive.h"

class TankItem : public QObject, public QGraphicsRectItem
{
	Q_OBJECT
	Q_INTERFACES(QGraphicsItem)

public:
	explicit TankItem(QObject *parent = 0);
	void setHorizontalAxis(DiveCartesianAxis *horizontal);
	void setData(DivePlotDataModel *model, struct plot_info *plotInfo, struct dive *d);

signals:

public slots:
	virtual void modelDataChanged(const QModelIndex &topLeft = QModelIndex(), const QModelIndex &bottomRight = QModelIndex());

private:
	DivePlotDataModel *dataModel;
	DiveCartesianAxis *hAxis;
	int hDataColumn;
	struct dive *dive;
	struct plot_info *pInfo;
	QList<QGraphicsRectItem *> rects;
};

#endif // TANKITEM_H
