#include "diveprofileitem.h"
#include "diveplotdatamodel.h"
#include "divecartesianaxis.h"
#include "graphicsview-common.h"
#include "divetextitem.h"
#include "profile.h"
#include "dive.h"
#include "profilegraphics.h"
#include "preferences.h"

#include <QPen>
#include <QPainter>
#include <QLinearGradient>
#include <QDebug>
#include <QApplication>
#include <QGraphicsItem>

AbstractProfilePolygonItem::AbstractProfilePolygonItem(): QObject(), QGraphicsPolygonItem(),
	hAxis(NULL), vAxis(NULL), dataModel(NULL), hDataColumn(-1), vDataColumn(-1)
{
	connect(PreferencesDialog::instance(), SIGNAL(settingsChanged()), this, SLOT(modelDataChanged()));
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

void AbstractProfilePolygonItem::setModel(DivePlotDataModel* model)
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

	qDeleteAll(texts);
	texts.clear();
}

void DiveProfileItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
	Q_UNUSED(widget);

	// This paints the Polygon + Background. I'm setting the pen to QPen() so we don't get a black line here,
	// after all we need to plot the correct velocities colors later.
	setPen(Qt::NoPen);
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

	/* Show any ceiling we may have encountered */
	if (prefs.profile_dc_ceiling) {
		QPolygonF p = polygon();
		plot_data *entry = dataModel->data() + dataModel->rowCount()-1;
		for (int i = dataModel->rowCount() - 1; i >= 0; i--, entry--) {
			if (!entry->in_deco) {
				/* not in deco implies this is a safety stop, no ceiling */
				p.append(QPointF(hAxis->posAtValue(entry->sec), vAxis->posAtValue(0)));
			} else if (entry->stopdepth < entry->depth) {
				p.append(QPointF(hAxis->posAtValue(entry->sec), vAxis->posAtValue(entry->stopdepth)));
			} else {
				p.append(QPointF(hAxis->posAtValue(entry->sec), vAxis->posAtValue(entry->depth)));
			}
		}
		setPolygon(p);
	}

		// This is the blueish gradient that the Depth Profile should have.
	// It's a simple QLinearGradient with 2 stops, starting from top to bottom.
	QLinearGradient pat(0, polygon().boundingRect().top(), 0, polygon().boundingRect().bottom());
	pat.setColorAt(1, getColor(DEPTH_BOTTOM));
	pat.setColorAt(0, getColor(DEPTH_TOP));
	setBrush(QBrush(pat));

	int last = -1;
	for (int i = 0, count  = dataModel->rowCount(); i < count; i++) {

		struct plot_data *entry = dataModel->data()+i;
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

	for (int i = 0, count = dataModel->rowCount(); i < count; i++) {
		plot_data* entry = dataModel->data() + i;
		int mbar = GET_PRESSURE(entry);

		if (entry->cylinderindex != last_index) {
			polygons.append(QPolygonF()); // this is the polygon that will be actually drawned on screen.
			last_index = entry->cylinderindex;
		}
		if (!mbar) {
			continue;
		}

		QPointF point(hAxis->posAtValue(entry->sec), vAxis->posAtValue(mbar));
		boundingPoly.push_back(point); // The BoundingRect
		polygons.last().push_back(point); // The polygon thta will be plotted.
	}
	setPolygon(boundingPoly);

	int mbar, cyl;
	int seen_cyl[MAX_CYLINDERS] = { false, };
	int last_pressure[MAX_CYLINDERS] = { 0, };
	int last_time[MAX_CYLINDERS] = { 0, };
	struct plot_data *entry;
	struct dive *dive = getDiveById(dataModel->id());
	Q_ASSERT(dive != NULL);

	cyl = -1;
	for (int i = 0, count = dataModel->rowCount(); i < count; i++) {
		entry = dataModel->data() + i;
		mbar = GET_PRESSURE(entry);

		if (!mbar)
			continue;
		if (cyl != entry->cylinderindex) {
			cyl = entry->cylinderindex;
			if (!seen_cyl[cyl]) {
				plot_pressure_value(mbar, entry->sec, Qt::AlignLeft | Qt::AlignBottom);
				plot_gas_value(mbar, entry->sec, Qt::AlignLeft | Qt::AlignTop,
						get_o2(&dive->cylinder[cyl].gasmix),
						get_he(&dive->cylinder[cyl].gasmix));
				seen_cyl[cyl] = true;
			}
		}
		last_pressure[cyl] = mbar;
		last_time[cyl] = entry->sec;
	}

	for (cyl = 0; cyl < MAX_CYLINDERS; cyl++) {
		if (last_time[cyl]) {
			plot_pressure_value(last_pressure[cyl], last_time[cyl], Qt::AlignHCenter | Qt::AlignTop);
		}
	}
}

void DiveGasPressureItem::plot_pressure_value(int mbar, int sec, QFlags<Qt::AlignmentFlag> flags)
{
	const char *unit;
	int pressure = get_pressure_units(mbar, &unit);
	DiveTextItem *text = new DiveTextItem(this);
	text->setPos(hAxis->posAtValue(sec), vAxis->posAtValue(mbar));
	text->setText(QString("%1 %2").arg(pressure).arg(unit));
	text->setAlignment(flags);
	text->setBrush(getColor(PRESSURE_TEXT));
	texts.push_back(text);
}

void DiveGasPressureItem::plot_gas_value(int mbar, int sec, QFlags<Qt::AlignmentFlag> flags, int o2, int he)
{
	QString gas  = (is_air(o2, he)) ? tr("air") :
					  (he == 0) ? QString(tr("EAN%1")).arg((o2 + 5) / 10) :
						      QString("%1/%2").arg((o2 + 5) / 10).arg((he + 5) / 10);
	DiveTextItem *text = new DiveTextItem(this);
	text->setPos(hAxis->posAtValue(sec), vAxis->posAtValue(mbar));
	text->setText(gas);
	text->setAlignment(flags);
	text->setBrush(getColor(PRESSURE_TEXT));
	texts.push_back(text);
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

void DiveCalculatedCeiling::modelDataChanged()
{
	// We don't have enougth data to calculate things, quit.
	if (!hAxis || !vAxis || !dataModel || hDataColumn == -1 || vDataColumn == -1)
		return;
	AbstractProfilePolygonItem::modelDataChanged();
	// Add 2 points to close the polygon.
	QPolygonF poly = polygon();
	QPointF p1 = poly.first();
	QPointF p2 = poly.last();

	poly.prepend(QPointF(p1.x(), vAxis->posAtValue(0)));
	poly.append(QPointF(p2.x(), vAxis->posAtValue(0)));
	setPolygon(poly);

	QLinearGradient pat(0, polygon().boundingRect().top(), 0, polygon().boundingRect().bottom());
	pat.setColorAt(0, getColor(CALC_CEILING_SHALLOW));
	pat.setColorAt(1, getColor(CALC_CEILING_DEEP));
	setPen(QPen(QBrush(Qt::NoBrush),0));
	setBrush(pat);
}

void DiveCalculatedCeiling::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	QGraphicsPolygonItem::paint(painter, option, widget);
}
