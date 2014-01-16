#include "diveprofileitem.h"
#include "diveplotdatamodel.h"
#include "divecartesianaxis.h"
#include "graphicsview-common.h"

#include <QPen>
#include <QPainter>
#include <QLinearGradient>
#include <QDebug>

AbstractProfilePolygonItem::AbstractProfilePolygonItem(): QObject(), QGraphicsPolygonItem(),
	hAxis(NULL), vAxis(NULL), dataModel(NULL), hDataColumn(-1), vDataColumn(-1)
{

}

void AbstractProfilePolygonItem::setHorizontalAxis(DiveCartesianAxis* horizontal)
{
	hAxis = horizontal;
	modelDataChanged();
}

void AbstractProfilePolygonItem::setHorizontalDataColumn(int column)
{
	hDataColumn = column;
	modelDataChanged();
}

void AbstractProfilePolygonItem::setModel(QAbstractTableModel* model)
{
	dataModel = model;
	modelDataChanged();
}

void AbstractProfilePolygonItem::setVerticalAxis(DiveCartesianAxis* vertical)
{
	vAxis = vertical;
	modelDataChanged();
}

void AbstractProfilePolygonItem::setVerticalDataColumn(int column)
{
	vDataColumn = column;
	modelDataChanged();
}

void AbstractProfilePolygonItem::modelDataChanged()
{
	// We don't have enougth data to calculate things, quit.
	if (!hAxis || !vAxis || !dataModel || hDataColumn == -1 || vDataColumn == -1)
		return;

	// Calculate the polygon. This is the polygon that will be painted on screen
	// on the ::paint method. Here we calculate the correct position of the points
	// regarting our cartesian plane ( made by the hAxis and vAxis ), the QPolygonF
	// is an array of QPointF's, so we basically get the point from the model, convert
	// to our coordinates, store. no painting is done here.
	QPolygonF poly;
	for (int i = 0, modelDataCount = dataModel->rowCount(); i < modelDataCount; i++) {
		qreal horizontalValue = dataModel->index(i, hDataColumn).data().toReal();
		qreal verticalValue = dataModel->index(i, vDataColumn).data().toReal();
		QPointF point( hAxis->posAtValue(horizontalValue), vAxis->posAtValue(verticalValue));
		poly.append(point);
	}
	setPolygon(poly);
}

void DiveProfileItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
	Q_UNUSED(widget);

	// This paints the Polygon + Background. I'm setting the pen to QPen() so we don't get a black line here,
	// after all we need to plot the correct velocities colors later.
	setPen(QPen());
	QGraphicsPolygonItem::paint(painter, option, widget);

	// Here we actually paint the boundaries of the Polygon using the colors that the model provides.
	// Those are the speed colors of the dives.
	QPen pen;
	pen.setCosmetic(true);
	pen.setWidth(2);
	// This paints the colors of the velocities.
	for (int i = 1, count = dataModel->rowCount(); i < count; i++) {
		QModelIndex colorIndex = dataModel->index(i, DivePlotDataModel::COLOR);
		pen.setBrush(QBrush(colorIndex.data(Qt::BackgroundRole).value<QColor>()));
		painter->setPen(pen);
		painter->drawLine(polygon()[i-1],polygon()[i]);
	}
}

void DiveProfileItem::modelDataChanged(){
	AbstractProfilePolygonItem::modelDataChanged();
	if(polygon().isEmpty())
		return;
		// This is the blueish gradient that the Depth Profile should have.
	// It's a simple QLinearGradient with 2 stops, starting from top to bottom.
	QLinearGradient pat(0, polygon().boundingRect().top(), 0, polygon().boundingRect().bottom());
	pat.setColorAt(1, getColor(DEPTH_BOTTOM));
	pat.setColorAt(0, getColor(DEPTH_TOP));
	setBrush(QBrush(pat));
}

DiveTemperatureItem::DiveTemperatureItem()
{
	QPen pen;
	pen.setBrush(QBrush(getColor(::TEMP_PLOT)));
	pen.setCosmetic(true);
	pen.setWidth(2);
	setPen(pen);
}

void DiveTemperatureItem::modelDataChanged()
{
	// We don't have enougth data to calculate things, quit.
	if (!hAxis || !vAxis || !dataModel || hDataColumn == -1 || vDataColumn == -1)
		return;

	// Ignore empty values. things do not look good with '0' as temperature in kelvin...
	QPolygonF poly;
	for (int i = 0, modelDataCount = dataModel->rowCount(); i < modelDataCount; i++) {
		qreal verticalValue = dataModel->index(i, vDataColumn).data().toReal();
		if(!verticalValue)
			continue;
		qreal horizontalValue = dataModel->index(i, hDataColumn).data().toReal();
		QPointF point( hAxis->posAtValue(horizontalValue), vAxis->posAtValue(verticalValue));
		poly.append(point);
	}
	setPolygon(poly);
}

void DiveTemperatureItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	painter->setPen(pen());
	painter->drawPolyline(polygon());
}
