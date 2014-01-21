#include "diveprofileitem.h"
#include "diveplotdatamodel.h"
#include "divecartesianaxis.h"
#include "graphicsview-common.h"
#include "divetextitem.h"
#include "profile.h"
#include "dive.h"
#include "profilegraphics.h"

#include <QPen>
#include <QPainter>
#include <QLinearGradient>
#include <QDebug>
#include <QApplication>
#include <QGraphicsItem>

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

	qDeleteAll(texts);
	texts.clear();

	int last = -1;
	for (int i = 0, count  = dataModel->rowCount(); i < count; i++) {

		struct plot_data *entry = static_cast<DivePlotDataModel*>(dataModel)->data()+i;
		if (entry->depth < 2000)
			continue;

		if ((entry == entry->max[2]) && entry->depth / 100 != last) {
			plot_depth_sample(entry, Qt::AlignHCenter | Qt::AlignTop, getColor(SAMPLE_DEEP));
			last = entry->depth / 100;
		}

		if ((entry == entry->min[2]) && entry->depth / 100 != last) {
			plot_depth_sample(entry, Qt::AlignHCenter | Qt::AlignBottom, getColor(SAMPLE_SHALLOW));
			last = entry->depth / 100;
		}

		if (entry->depth != last)
			last = -1;
	}
}

void DiveProfileItem::plot_depth_sample(struct plot_data *entry,QFlags<Qt::AlignmentFlag> flags,const QColor& color)
{
	int decimals;
	double d = get_depth_units(entry->depth, &decimals, NULL);
	DiveTextItem *item = new DiveTextItem(this);
	item->setPos(hAxis->posAtValue(entry->sec), vAxis->posAtValue(entry->depth));
	item->setText(QString("%1").arg(d, 0, 'f', 1));
	item->setAlignment(flags);
	item->setBrush(color);
	texts.append(item);
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

	qDeleteAll(texts);
	texts.clear();
	// Ignore empty values. things do not look good with '0' as temperature in kelvin...
	QPolygonF poly;
	int last = -300, last_printed_temp = 0, sec = 0;
	for (int i = 0, modelDataCount = dataModel->rowCount(); i < modelDataCount; i++) {
		int mkelvin = dataModel->index(i, vDataColumn).data().toInt();
		if(!mkelvin)
			continue;
		int sec = dataModel->index(i, hDataColumn).data().toInt();
		QPointF point( hAxis->posAtValue(sec), vAxis->posAtValue(mkelvin));
		poly.append(point);

		/* don't print a temperature
		 * if it's been less than 5min and less than a 2K change OR
		 * if it's been less than 2min OR if the change from the
		 * last print is less than .4K (and therefore less than 1F) */
		if (((sec < last + 300) && (abs(mkelvin - last_printed_temp) < 2000)) ||
		    (sec < last + 120) ||
		    (abs(mkelvin - last_printed_temp) < 400))
			continue;
		last = sec;
		if (mkelvin > 200000)
			createTextItem(sec,mkelvin);
		last_printed_temp = mkelvin;
	}
	setPolygon(poly);

	/* it would be nice to print the end temperature, if it's
	* different or if the last temperature print has been more
	* than a quarter of the dive back */
	int last_temperature = dataModel->data(dataModel->index(dataModel->rowCount()-1, DivePlotDataModel::TEMPERATURE)).toInt();
	if (last_temperature > 200000 && ((abs(last_temperature - last_printed_temp) > 500) || ((double)last / (double)sec < 0.75))){
		createTextItem(sec, last_temperature);
	}
}

void DiveTemperatureItem::createTextItem(int sec, int mkelvin)
{
	double deg;
	const char *unit;
	static text_render_options_t tro = {TEMP_TEXT_SIZE, TEMP_TEXT};
	deg = get_temp_units(mkelvin, &unit);

	DiveTextItem *text = new DiveTextItem(this);
	text->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
	text->setBrush(getColor(TEMP_TEXT));
	text->setPos(QPointF(hAxis->posAtValue(sec), vAxis->posAtValue(mkelvin)));
	text->setText(QString("%1%2").arg(deg, 0, 'f', 1).arg(unit));
	// text->setSize(TEMP_TEXT_SIZE); //TODO: TEXT SIZE!
}

void DiveTemperatureItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	painter->setPen(pen());
	painter->drawPolyline(polygon());
}


void DiveGasPressureItem::modelDataChanged()
{
	// We don't have enougth data to calculate things, quit.
	if (!hAxis || !vAxis || !dataModel || hDataColumn == -1 || vDataColumn == -1)
		return;
	int last_index = -1;
	int lift_pen = false;
	int first_plot = true;
	QPolygonF boundingPoly; // This is the "Whole Item", but a pressure can be divided in N Polygons.
	polygons.clear();

	for (int i = 0; i < dataModel->rowCount(); i++) {
		int sPressure = dataModel->index(i, DivePlotDataModel::SENSOR_PRESSURE).data().toInt();
		int iPressure = dataModel->index(i, DivePlotDataModel::INTERPOLATED_PRESSURE).data().toInt();
		int cylIndex = dataModel->index(i, DivePlotDataModel::CYLINDERINDEX).data().toInt();
		int sec = dataModel->index(i, DivePlotDataModel::TIME).data().toInt();
		int mbar = sPressure ? sPressure : iPressure;

		if (cylIndex != last_index) {
			polygons.append(QPolygonF()); // this is the polygon that will be actually drawned on screen.
			last_index = cylIndex;
		}
		if (!mbar) {
			continue;
		}

		QPointF point(hAxis->posAtValue(sec), vAxis->posAtValue(mbar));
		boundingPoly.push_back(point); // The BoundingRect
		polygons.last().push_back(point); // The polygon thta will be plotted.
	}
	setPolygon(boundingPoly);
}

void DiveGasPressureItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	QPen pen;
	pen.setCosmetic(true);
	pen.setWidth(2);
	Q_FOREACH(const QPolygonF& poly, polygons){
		for (int i = 1, count = poly.count(); i < count; i++) {
			pen.setBrush(QBrush(Qt::red)); // TODO: Fix the color.
			painter->setPen(pen);
			painter->drawLine(poly[i-1],poly[i]);
		}
	}
}
