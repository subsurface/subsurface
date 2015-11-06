#include "diveprofileitem.h"
#include "diveplotdatamodel.h"
#include "divecartesianaxis.h"
#include "divetextitem.h"
#include "animationfunctions.h"
#include "dive.h"
#include "profile.h"
#ifndef SUBSURFACE_MOBILE
#include "preferences/preferencesdialog.h"
#endif
#include "diveplannermodel.h"
#include "helpers.h"
#include "libdivecomputer/parser.h"
#include "profilewidget2.h"

#include <QSettings>

AbstractProfilePolygonItem::AbstractProfilePolygonItem() : QObject(), QGraphicsPolygonItem(), hAxis(NULL), vAxis(NULL), dataModel(NULL), hDataColumn(-1), vDataColumn(-1)
{
	setCacheMode(DeviceCoordinateCache);
#ifndef SUBSURFACE_MOBILE
	connect(PreferencesDialog::instance(), SIGNAL(settingsChanged()), this, SLOT(settingsChanged()));
#endif
}

void AbstractProfilePolygonItem::settingsChanged()
{
}

void AbstractProfilePolygonItem::setHorizontalAxis(DiveCartesianAxis *horizontal)
{
	hAxis = horizontal;
	connect(hAxis, SIGNAL(sizeChanged()), this, SLOT(modelDataChanged()));
	modelDataChanged();
}

void AbstractProfilePolygonItem::setHorizontalDataColumn(int column)
{
	hDataColumn = column;
	modelDataChanged();
}

void AbstractProfilePolygonItem::setModel(DivePlotDataModel *model)
{
	dataModel = model;
	connect(dataModel, SIGNAL(dataChanged(QModelIndex, QModelIndex)), this, SLOT(modelDataChanged(QModelIndex, QModelIndex)));
	connect(dataModel, SIGNAL(rowsAboutToBeRemoved(QModelIndex, int, int)), this, SLOT(modelDataRemoved(QModelIndex, int, int)));
	modelDataChanged();
}

void AbstractProfilePolygonItem::modelDataRemoved(const QModelIndex &parent, int from, int to)
{
	setPolygon(QPolygonF());
	qDeleteAll(texts);
	texts.clear();
}

void AbstractProfilePolygonItem::setVerticalAxis(DiveCartesianAxis *vertical)
{
	vAxis = vertical;
	connect(vAxis, SIGNAL(sizeChanged()), this, SLOT(modelDataChanged()));
	connect(vAxis, SIGNAL(maxChanged()), this, SLOT(modelDataChanged()));
	modelDataChanged();
}

void AbstractProfilePolygonItem::setVerticalDataColumn(int column)
{
	vDataColumn = column;
	modelDataChanged();
}

bool AbstractProfilePolygonItem::shouldCalculateStuff(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
	if (!hAxis || !vAxis)
		return false;
	if (!dataModel || dataModel->rowCount() == 0)
		return false;
	if (hDataColumn == -1 || vDataColumn == -1)
		return false;
	if (topLeft.isValid() && bottomRight.isValid()) {
		if ((topLeft.column() >= vDataColumn || topLeft.column() >= hDataColumn) &&
		    (bottomRight.column() <= vDataColumn || topLeft.column() <= hDataColumn)) {
			return true;
		}
	}
	return true;
}

void AbstractProfilePolygonItem::modelDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
	// We don't have enougth data to calculate things, quit.

	// Calculate the polygon. This is the polygon that will be painted on screen
	// on the ::paint method. Here we calculate the correct position of the points
	// regarting our cartesian plane ( made by the hAxis and vAxis ), the QPolygonF
	// is an array of QPointF's, so we basically get the point from the model, convert
	// to our coordinates, store. no painting is done here.
	QPolygonF poly;
	for (int i = 0, modelDataCount = dataModel->rowCount(); i < modelDataCount; i++) {
		qreal horizontalValue = dataModel->index(i, hDataColumn).data().toReal();
		qreal verticalValue = dataModel->index(i, vDataColumn).data().toReal();
		QPointF point(hAxis->posAtValue(horizontalValue), vAxis->posAtValue(verticalValue));
		poly.append(point);
	}
	setPolygon(poly);

	qDeleteAll(texts);
	texts.clear();
}

DiveProfileItem::DiveProfileItem() : show_reported_ceiling(0), reported_ceiling_in_red(0)
{
}

void DiveProfileItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	Q_UNUSED(widget);
	if (polygon().isEmpty())
		return;

	painter->save();
	// This paints the Polygon + Background. I'm setting the pen to QPen() so we don't get a black line here,
	// after all we need to plot the correct velocities colors later.
	setPen(Qt::NoPen);
	QGraphicsPolygonItem::paint(painter, option, widget);

	// Here we actually paint the boundaries of the Polygon using the colors that the model provides.
	// Those are the speed colors of the dives.
	QPen pen;
	pen.setCosmetic(true);
	pen.setWidth(2);
	QPolygonF poly = polygon();
	// This paints the colors of the velocities.
	for (int i = 1, count = dataModel->rowCount(); i < count; i++) {
		QModelIndex colorIndex = dataModel->index(i, DivePlotDataModel::COLOR);
		pen.setBrush(QBrush(colorIndex.data(Qt::BackgroundRole).value<QColor>()));
		painter->setPen(pen);
		painter->drawLine(poly[i - 1], poly[i]);
	}
	painter->restore();
}

int DiveProfileItem::maxCeiling(int row)
{
	int max = -1;
	plot_data *entry = dataModel->data().entry + row;
	for (int tissue = 0; tissue < 16; tissue++) {
		if (max < entry->ceilings[tissue])
			max = entry->ceilings[tissue];
	}
	return max;
}

void DiveProfileItem::modelDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
	bool eventAdded = false;
	if (!shouldCalculateStuff(topLeft, bottomRight))
		return;

	AbstractProfilePolygonItem::modelDataChanged(topLeft, bottomRight);
	if (polygon().isEmpty())
		return;

	show_reported_ceiling = prefs.dcceiling;
	reported_ceiling_in_red = prefs.redceiling;
	profileColor = getColor(DEPTH_BOTTOM);

	int currState = qobject_cast<ProfileWidget2 *>(scene()->views().first())->currentState;
	if (currState == ProfileWidget2::PLAN) {
		plot_data *entry = dataModel->data().entry;
		for (int i = 0; i < dataModel->rowCount(); i++, entry++) {
			int max = maxCeiling(i);
			// Don't scream if we violate the ceiling by a few cm
			if (entry->depth < max - 100 && entry->sec > 0) {
				profileColor = QColor(Qt::red);
				if (!eventAdded) {
					add_event(&displayed_dive.dc, entry->sec, SAMPLE_EVENT_CEILING, -1, max / 1000, "planned waypoint above ceiling");
					eventAdded = true;
				}
			}
		}
	}

	/* Show any ceiling we may have encountered */
	if (prefs.dcceiling && !prefs.redceiling) {
		QPolygonF p = polygon();
		plot_data *entry = dataModel->data().entry + dataModel->rowCount() - 1;
		for (int i = dataModel->rowCount() - 1; i >= 0; i--, entry--) {
			if (!entry->in_deco) {
				/* not in deco implies this is a safety stop, no ceiling */
				p.append(QPointF(hAxis->posAtValue(entry->sec), vAxis->posAtValue(0)));
			} else {
				p.append(QPointF(hAxis->posAtValue(entry->sec), vAxis->posAtValue(qMin(entry->stopdepth, entry->depth))));
			}
		}
		setPolygon(p);
	}

	// This is the blueish gradient that the Depth Profile should have.
	// It's a simple QLinearGradient with 2 stops, starting from top to bottom.
	QLinearGradient pat(0, polygon().boundingRect().top(), 0, polygon().boundingRect().bottom());
	pat.setColorAt(1, profileColor);
	pat.setColorAt(0, getColor(DEPTH_TOP));
	setBrush(QBrush(pat));

	int last = -1;
	for (int i = 0, count = dataModel->rowCount(); i < count; i++) {

		struct plot_data *entry = dataModel->data().entry + i;
		if (entry->depth < 2000)
			continue;

		if ((entry == entry->max[2]) && entry->depth / 100 != last) {
			plot_depth_sample(entry, Qt::AlignHCenter | Qt::AlignBottom, getColor(SAMPLE_DEEP));
			last = entry->depth / 100;
		}

		if ((entry == entry->min[2]) && entry->depth / 100 != last) {
			plot_depth_sample(entry, Qt::AlignHCenter | Qt::AlignTop, getColor(SAMPLE_SHALLOW));
			last = entry->depth / 100;
		}

		if (entry->depth != last)
			last = -1;
	}
}

void DiveProfileItem::settingsChanged()
{
	//TODO: Only modelDataChanged() here if we need to rebuild the graph ( for instance,
	// if the prefs.dcceiling are enabled, but prefs.redceiling is disabled
	// and only if it changed something. let's not waste cpu cycles repoloting something we don't need to.
	modelDataChanged();
}

void DiveProfileItem::plot_depth_sample(struct plot_data *entry, QFlags<Qt::AlignmentFlag> flags, const QColor &color)
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

DiveHeartrateItem::DiveHeartrateItem()
{
	QPen pen;
	pen.setBrush(QBrush(getColor(::HR_PLOT)));
	pen.setCosmetic(true);
	pen.setWidth(1);
	setPen(pen);
	settingsChanged();
}

void DiveHeartrateItem::modelDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
	int last = -300, last_printed_hr = 0, sec = 0;
	struct {
		int sec;
		int hr;
	} hist[3] = {};

	// We don't have enougth data to calculate things, quit.
	if (!shouldCalculateStuff(topLeft, bottomRight))
		return;

	qDeleteAll(texts);
	texts.clear();
	// Ignore empty values. a heartrate of 0 would be a bad sign.
	QPolygonF poly;
	for (int i = 0, modelDataCount = dataModel->rowCount(); i < modelDataCount; i++) {
		int hr = dataModel->index(i, vDataColumn).data().toInt();
		if (!hr)
			continue;
		sec = dataModel->index(i, hDataColumn).data().toInt();
		QPointF point(hAxis->posAtValue(sec), vAxis->posAtValue(hr));
		poly.append(point);
		if (hr == hist[2].hr)
			// same as last one, no point in looking at printing
			continue;
		hist[0] = hist[1];
		hist[1] = hist[2];
		hist[2].sec = sec;
		hist[2].hr = hr;
		// don't print a HR
		// if it's not a local min / max
		// if it's been less than 5min and less than a 20 beats change OR
		// if it's been less than 2min OR if the change from the
		// last print is less than 10 beats
		// to test min / max requires three points, so we now look at the
		// previous one
		sec = hist[1].sec;
		hr = hist[1].hr;
		if ((hist[0].hr < hr && hr < hist[2].hr) ||
		    (hist[0].hr > hr && hr > hist[2].hr) ||
		    ((sec < last + 300) && (abs(hr - last_printed_hr) < 20)) ||
		    (sec < last + 120) ||
		    (abs(hr - last_printed_hr) < 10))
			continue;
		last = sec;
		createTextItem(sec, hr);
		last_printed_hr = hr;
	}
	setPolygon(poly);

	if (texts.count())
		texts.last()->setAlignment(Qt::AlignLeft | Qt::AlignBottom);
}

void DiveHeartrateItem::createTextItem(int sec, int hr)
{
	DiveTextItem *text = new DiveTextItem(this);
	text->setAlignment(Qt::AlignRight | Qt::AlignBottom);
	text->setBrush(getColor(HR_TEXT));
	text->setPos(QPointF(hAxis->posAtValue(sec), vAxis->posAtValue(hr)));
	text->setScale(0.7); // need to call this BEFORE setText()
	text->setText(QString("%1").arg(hr));
	texts.append(text);
}

void DiveHeartrateItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	if (polygon().isEmpty())
		return;
	painter->save();
	painter->setPen(pen());
	painter->drawPolyline(polygon());
	painter->restore();
}

void DiveHeartrateItem::settingsChanged()
{
	setVisible(prefs.hrgraph);
}

DivePercentageItem::DivePercentageItem(int i)
{
	QPen pen;
	QColor color;
	color.setHsl(100 + 10 * i, 200, 100);
	pen.setBrush(QBrush(color));
	pen.setCosmetic(true);
	pen.setWidth(1);
	setPen(pen);
	settingsChanged();
}

void DivePercentageItem::modelDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
	int sec = 0;

	// We don't have enougth data to calculate things, quit.
	if (!shouldCalculateStuff(topLeft, bottomRight))
		return;

	// Ignore empty values. a heartrate of 0 would be a bad sign.
	QPolygonF poly;
	for (int i = 0, modelDataCount = dataModel->rowCount(); i < modelDataCount; i++) {
		int hr = dataModel->index(i, vDataColumn).data().toInt();
		if (!hr)
			continue;
		sec = dataModel->index(i, hDataColumn).data().toInt();
		QPointF point(hAxis->posAtValue(sec), vAxis->posAtValue(hr));
		poly.append(point);
	}
	setPolygon(poly);

	if (texts.count())
		texts.last()->setAlignment(Qt::AlignLeft | Qt::AlignBottom);
}

void DivePercentageItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	if (polygon().isEmpty())
		return;
	painter->save();
	painter->setPen(pen());
	painter->drawPolyline(polygon());
	painter->restore();
}

void DivePercentageItem::settingsChanged()
{
	setVisible(prefs.percentagegraph);
}

DiveAmbPressureItem::DiveAmbPressureItem()
{
	QPen pen;
	pen.setBrush(QBrush(getColor(::AMB_PRESSURE_LINE)));
	pen.setCosmetic(true);
	pen.setWidth(2);
	setPen(pen);
	settingsChanged();
}

void DiveAmbPressureItem::modelDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
	int sec = 0;

	// We don't have enougth data to calculate things, quit.
	if (!shouldCalculateStuff(topLeft, bottomRight))
		return;

	// Ignore empty values. a heartrate of 0 would be a bad sign.
	QPolygonF poly;
	for (int i = 0, modelDataCount = dataModel->rowCount(); i < modelDataCount; i++) {
		int hr = dataModel->index(i, vDataColumn).data().toInt();
		if (!hr)
			continue;
		sec = dataModel->index(i, hDataColumn).data().toInt();
		QPointF point(hAxis->posAtValue(sec), vAxis->posAtValue(hr));
		poly.append(point);
	}
	setPolygon(poly);

	if (texts.count())
		texts.last()->setAlignment(Qt::AlignLeft | Qt::AlignBottom);
}

void DiveAmbPressureItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	if (polygon().isEmpty())
		return;
	painter->save();
	painter->setPen(pen());
	painter->drawPolyline(polygon());
	painter->restore();
}

void DiveAmbPressureItem::settingsChanged()
{
	setVisible(prefs.percentagegraph);
}

DiveGFLineItem::DiveGFLineItem()
{
	QPen pen;
	pen.setBrush(QBrush(getColor(::GF_LINE)));
	pen.setCosmetic(true);
	pen.setWidth(2);
	setPen(pen);
	settingsChanged();
}

void DiveGFLineItem::modelDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
	int sec = 0;

	// We don't have enougth data to calculate things, quit.
	if (!shouldCalculateStuff(topLeft, bottomRight))
		return;

	// Ignore empty values. a heartrate of 0 would be a bad sign.
	QPolygonF poly;
	for (int i = 0, modelDataCount = dataModel->rowCount(); i < modelDataCount; i++) {
		int hr = dataModel->index(i, vDataColumn).data().toInt();
		if (!hr)
			continue;
		sec = dataModel->index(i, hDataColumn).data().toInt();
		QPointF point(hAxis->posAtValue(sec), vAxis->posAtValue(hr));
		poly.append(point);
	}
	setPolygon(poly);

	if (texts.count())
		texts.last()->setAlignment(Qt::AlignLeft | Qt::AlignBottom);
}

void DiveGFLineItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	if (polygon().isEmpty())
		return;
	painter->save();
	painter->setPen(pen());
	painter->drawPolyline(polygon());
	painter->restore();
}

void DiveGFLineItem::settingsChanged()
{
	setVisible(prefs.percentagegraph);
}

DiveTemperatureItem::DiveTemperatureItem()
{
	QPen pen;
	pen.setBrush(QBrush(getColor(::TEMP_PLOT)));
	pen.setCosmetic(true);
	pen.setWidth(2);
	setPen(pen);
}

void DiveTemperatureItem::modelDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
	int last = -300, last_printed_temp = 0, sec = 0, last_valid_temp = 0;
	// We don't have enougth data to calculate things, quit.
	if (!shouldCalculateStuff(topLeft, bottomRight))
		return;

	qDeleteAll(texts);
	texts.clear();
	// Ignore empty values. things do not look good with '0' as temperature in kelvin...
	QPolygonF poly;
	for (int i = 0, modelDataCount = dataModel->rowCount(); i < modelDataCount; i++) {
		int mkelvin = dataModel->index(i, vDataColumn).data().toInt();
		if (!mkelvin)
			continue;
		last_valid_temp = mkelvin;
		sec = dataModel->index(i, hDataColumn).data().toInt();
		QPointF point(hAxis->posAtValue(sec), vAxis->posAtValue(mkelvin));
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
			createTextItem(sec, mkelvin);
		last_printed_temp = mkelvin;
	}
	setPolygon(poly);

	/* it would be nice to print the end temperature, if it's
	* different or if the last temperature print has been more
	* than a quarter of the dive back */
	if (last_valid_temp > 200000 &&
	    ((abs(last_valid_temp - last_printed_temp) > 500) || ((double)last / (double)sec < 0.75))) {
		createTextItem(sec, last_valid_temp);
	}
	if (texts.count())
		texts.last()->setAlignment(Qt::AlignLeft | Qt::AlignBottom);
}

void DiveTemperatureItem::createTextItem(int sec, int mkelvin)
{
	double deg;
	const char *unit;
	deg = get_temp_units(mkelvin, &unit);

	DiveTextItem *text = new DiveTextItem(this);
	text->setAlignment(Qt::AlignRight | Qt::AlignBottom);
	text->setBrush(getColor(TEMP_TEXT));
	text->setPos(QPointF(hAxis->posAtValue(sec), vAxis->posAtValue(mkelvin)));
	text->setScale(0.8); // need to call this BEFORE setText()
	text->setText(QString("%1%2").arg(deg, 0, 'f', 1).arg(unit));
	texts.append(text);
}

void DiveTemperatureItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	if (polygon().isEmpty())
		return;
	painter->save();
	painter->setPen(pen());
	painter->drawPolyline(polygon());
	painter->restore();
}

DiveMeanDepthItem::DiveMeanDepthItem()
{
	QPen pen;
	pen.setBrush(QBrush(getColor(::HR_AXIS)));
	pen.setCosmetic(true);
	pen.setWidth(2);
	setPen(pen);
	settingsChanged();
}

void DiveMeanDepthItem::modelDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
	double meandepthvalue = 0.0;
	// We don't have enougth data to calculate things, quit.
	if (!shouldCalculateStuff(topLeft, bottomRight))
		return;

	QPolygonF poly;
	plot_data *entry = dataModel->data().entry;
	for (int i = 0, modelDataCount = dataModel->rowCount(); i < modelDataCount; i++, entry++) {
		// Ignore empty values
		if (entry->running_sum == 0 || entry->sec == 0)
			continue;

		meandepthvalue = entry->running_sum / entry->sec;
		QPointF point(hAxis->posAtValue(entry->sec), vAxis->posAtValue(meandepthvalue));
		poly.append(point);
	}
	lastRunningSum = meandepthvalue;
	setPolygon(poly);
	createTextItem();
}


void DiveMeanDepthItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	if (polygon().isEmpty())
		return;
	painter->save();
	painter->setPen(pen());
	painter->drawPolyline(polygon());
	painter->restore();
}

void DiveMeanDepthItem::settingsChanged()
{
	setVisible(prefs.show_average_depth);
}

void DiveMeanDepthItem::createTextItem() {
	plot_data *entry = dataModel->data().entry;
	int sec = entry[dataModel->rowCount()-1].sec;
	qDeleteAll(texts);
	texts.clear();
	int decimals;
	const char *unitText;
	double d = get_depth_units(lastRunningSum, &decimals, &unitText);
	DiveTextItem *text = new DiveTextItem(this);
	text->setAlignment(Qt::AlignRight | Qt::AlignTop);
	text->setBrush(getColor(TEMP_TEXT));
	text->setPos(QPointF(hAxis->posAtValue(sec) + 1, vAxis->posAtValue(lastRunningSum)));
	text->setScale(0.8); // need to call this BEFORE setText()
	text->setText(QString("%1%2").arg(d, 0, 'f', 1).arg(unitText));
	texts.append(text);
}

void DiveGasPressureItem::modelDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
	// We don't have enougth data to calculate things, quit.
	if (!shouldCalculateStuff(topLeft, bottomRight))
		return;
	int last_index = -1;
	int o2mbar;
	QPolygonF boundingPoly, o2Poly; // This is the "Whole Item", but a pressure can be divided in N Polygons.
	polygons.clear();
	if (displayed_dive.dc.divemode == CCR)
		polygons.append(o2Poly);

	for (int i = 0, count = dataModel->rowCount(); i < count; i++) {
		o2mbar = 0;
		plot_data *entry = dataModel->data().entry + i;
		int mbar = GET_PRESSURE(entry);
		if (displayed_dive.dc.divemode == CCR)
			o2mbar = GET_O2CYLINDER_PRESSURE(entry);

		if (entry->cylinderindex != last_index) {
			polygons.append(QPolygonF()); // this is the polygon that will be actually drawn on screen.
			last_index = entry->cylinderindex;
		}
		if (!mbar) {
			continue;
		}
		if (o2mbar) {
			QPointF o2point(hAxis->posAtValue(entry->sec), vAxis->posAtValue(o2mbar));
			boundingPoly.push_back(o2point);
			polygons.first().push_back(o2point);
		}

		QPointF point(hAxis->posAtValue(entry->sec), vAxis->posAtValue(mbar));
		boundingPoly.push_back(point);    // The BoundingRect
		polygons.last().push_back(point); // The polygon thta will be plotted.
	}
	setPolygon(boundingPoly);
	qDeleteAll(texts);
	texts.clear();
	int mbar, cyl;
	int seen_cyl[MAX_CYLINDERS] = { false, };
	int last_pressure[MAX_CYLINDERS] = { 0, };
	int last_time[MAX_CYLINDERS] = { 0, };
	struct plot_data *entry;

	cyl = -1;
	o2mbar = 0;

	double print_y_offset[8][2] = { { 0, -0.5 }, { 0, -0.5 }, { 0, -0.5 }, { 0, -0.5 }, { 0, -0.5 } ,{ 0, -0.5 }, { 0, -0.5 }, { 0, -0.5 } };
	// CCR dives: These are offset values used to print the gas lables and pressures on a CCR dive profile at
	// appropriate Y-coordinates: One doublet of values for each of 8 cylinders.
	// Order of offsets within a doublet: gas lable offset; gas pressure offset.
	// The array is initialised with default values that apply to non-CCR dives.

	bool offsets_initialised = false;
	int o2cyl = -1, dilcyl = -1;
	QFlags<Qt::AlignmentFlag> alignVar= Qt::AlignTop, align_dil = Qt::AlignBottom, align_o2 = Qt::AlignTop;
	double axisRange = (vAxis->maximum() - vAxis->minimum())/1000;	// Convert axis pressure range to bar
	double axisLog = log10(log10(axisRange));
	for (int i = 0, count = dataModel->rowCount(); i < count; i++) {
		entry = dataModel->data().entry + i;
		mbar = GET_PRESSURE(entry);
		if (displayed_dive.dc.divemode == CCR && displayed_dive.oxygen_cylinder_index >= 0)
			o2mbar = GET_O2CYLINDER_PRESSURE(entry);

		if (o2mbar) {	// If there is an o2mbar value then this is a CCR dive. Then do:
			// The first time an o2 value is detected, see if the oxygen cyl pressure graph starts above or below the dil graph
			if (!offsets_initialised) {	// Initialise the parameters for placing the text correctly near the graph line:
				o2cyl = displayed_dive.oxygen_cylinder_index;
				dilcyl = displayed_dive.diluent_cylinder_index;
				if ((o2mbar > mbar)) {	// If above, write o2 start cyl pressure above graph and diluent pressure below graph:
					print_y_offset[o2cyl][0] = -7 * axisLog; // y offset for oxygen gas lable (above); pressure offsets=-0.5, already initialised
					print_y_offset[dilcyl][0] = 5 * axisLog; // y offset for diluent gas lable (below)
				} else {				// ... else write o2 start cyl pressure below graph:
					print_y_offset[o2cyl][0]  = 5 * axisLog; // o2 lable & pressure below graph; pressure offsets=-0.5, already initialised
					print_y_offset[dilcyl][0] = -7.8 * axisLog;  // and diluent lable above graph.
					align_dil = Qt::AlignTop;
					align_o2  = Qt::AlignBottom;
				}
				offsets_initialised = true;
			}

			if (!seen_cyl[displayed_dive.oxygen_cylinder_index]) {	//For o2, on the left of profile, write lable and pressure
				plotPressureValue(o2mbar, entry->sec, align_o2, print_y_offset[o2cyl][1]);
				plotGasValue(o2mbar, entry->sec, displayed_dive.cylinder[displayed_dive.oxygen_cylinder_index].gasmix, align_o2, print_y_offset[o2cyl][0]);
				seen_cyl[displayed_dive.oxygen_cylinder_index] = true;
			}
			last_pressure[displayed_dive.oxygen_cylinder_index] = o2mbar;
			last_time[displayed_dive.oxygen_cylinder_index] = entry->sec;
			alignVar = align_dil;
		}

		if (!mbar)
			continue;

		if (cyl != entry->cylinderindex) {	// Pressure value near the left hand edge of the profile - other cylinders:
			cyl = entry->cylinderindex;	// For each other cylinder, write the gas lable and pressure
			if (!seen_cyl[cyl]) {
				plotPressureValue(mbar, entry->sec, alignVar, print_y_offset[cyl][1]);
				plotGasValue(mbar, entry->sec, displayed_dive.cylinder[cyl].gasmix, align_dil, print_y_offset[cyl][0]);
				seen_cyl[cyl] = true;
			}
		}
		last_pressure[cyl] = mbar;
		last_time[cyl] = entry->sec;
	}

	for (cyl = 0; cyl < MAX_CYLINDERS; cyl++) {	// For each cylinder, on right hand side of profile, write cylinder pressure
		alignVar = ((o2cyl >= 0) && (cyl == displayed_dive.oxygen_cylinder_index)) ? align_o2 : align_dil;
		if (last_time[cyl]) {
			plotPressureValue(last_pressure[cyl], last_time[cyl], (alignVar | Qt::AlignLeft), print_y_offset[cyl][1]);
		}
	}
}

void DiveGasPressureItem::plotPressureValue(int mbar, int sec, QFlags<Qt::AlignmentFlag> align, double pressure_offset)
{
	const char *unit;
	int pressure = get_pressure_units(mbar, &unit);
	DiveTextItem *text = new DiveTextItem(this);
	text->setPos(hAxis->posAtValue(sec), vAxis->posAtValue(mbar) + pressure_offset );
	text->setText(QString("%1 %2").arg(pressure).arg(unit));
	text->setAlignment(align);
	text->setBrush(getColor(PRESSURE_TEXT));
	texts.push_back(text);
}

void DiveGasPressureItem::plotGasValue(int mbar, int sec, struct gasmix gasmix, QFlags<Qt::AlignmentFlag> align, double gasname_offset)
{
	QString gas = get_gas_string(gasmix);
	DiveTextItem *text = new DiveTextItem(this);
	text->setPos(hAxis->posAtValue(sec), vAxis->posAtValue(mbar) + gasname_offset );
	text->setText(gas);
	text->setAlignment(align);
	text->setBrush(getColor(PRESSURE_TEXT));
	texts.push_back(text);
}

void DiveGasPressureItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	if (polygon().isEmpty())
		return;
	QPen pen;
	pen.setCosmetic(true);
	pen.setWidth(2);
	painter->save();
	struct plot_data *entry;
	Q_FOREACH (const QPolygonF &poly, polygons) {
		entry = dataModel->data().entry;
		for (int i = 1, count = poly.count(); i < count; i++, entry++) {
			if (entry->sac)
				pen.setBrush(getSacColor(entry->sac, displayed_dive.sac));
			else
				pen.setBrush(MED_GRAY_HIGH_TRANS);
			painter->setPen(pen);
			painter->drawLine(poly[i - 1], poly[i]);
		}
	}
	painter->restore();
}

DiveCalculatedCeiling::DiveCalculatedCeiling(ProfileWidget2 *widget) :
	profileWidget(widget),
	is3mIncrement(false)
{
	settingsChanged();
}

void DiveCalculatedCeiling::modelDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
	connect(profileWidget, SIGNAL(dateTimeChangedItems()), this, SLOT(recalc()), Qt::UniqueConnection);

	// We don't have enougth data to calculate things, quit.
	if (!shouldCalculateStuff(topLeft, bottomRight))
		return;
	AbstractProfilePolygonItem::modelDataChanged(topLeft, bottomRight);
	// Add 2 points to close the polygon.
	QPolygonF poly = polygon();
	if (poly.isEmpty())
		return;
	QPointF p1 = poly.first();
	QPointF p2 = poly.last();

	poly.prepend(QPointF(p1.x(), vAxis->posAtValue(0)));
	poly.append(QPointF(p2.x(), vAxis->posAtValue(0)));
	setPolygon(poly);

	QLinearGradient pat(0, polygon().boundingRect().top(), 0, polygon().boundingRect().bottom());
	pat.setColorAt(0, getColor(CALC_CEILING_SHALLOW));
	pat.setColorAt(1, getColor(CALC_CEILING_DEEP));
	setPen(QPen(QBrush(Qt::NoBrush), 0));
	setBrush(pat);
}

void DiveCalculatedCeiling::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	if (polygon().isEmpty())
		return;
	QGraphicsPolygonItem::paint(painter, option, widget);
}

DiveCalculatedTissue::DiveCalculatedTissue(ProfileWidget2 *widget) : DiveCalculatedCeiling(widget)
{
	settingsChanged();
}

void DiveCalculatedTissue::settingsChanged()
{
	setVisible(prefs.calcalltissues && prefs.calcceiling);
}

void DiveReportedCeiling::modelDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
	if (!shouldCalculateStuff(topLeft, bottomRight))
		return;

	QPolygonF p;
	p.append(QPointF(hAxis->posAtValue(0), vAxis->posAtValue(0)));
	plot_data *entry = dataModel->data().entry;
	for (int i = 0, count = dataModel->rowCount(); i < count; i++, entry++) {
		if (entry->in_deco && entry->stopdepth) {
			p.append(QPointF(hAxis->posAtValue(entry->sec), vAxis->posAtValue(qMin(entry->stopdepth, entry->depth))));
		} else {
			p.append(QPointF(hAxis->posAtValue(entry->sec), vAxis->posAtValue(0)));
		}
	}
	setPolygon(p);
	QLinearGradient pat(0, p.boundingRect().top(), 0, p.boundingRect().bottom());
	// does the user want the ceiling in "surface color" or in red?
	if (prefs.redceiling) {
		pat.setColorAt(0, getColor(CEILING_SHALLOW));
		pat.setColorAt(1, getColor(CEILING_DEEP));
	} else {
		pat.setColorAt(0, getColor(BACKGROUND_TRANS));
		pat.setColorAt(1, getColor(BACKGROUND_TRANS));
	}
	setPen(QPen(QBrush(Qt::NoBrush), 0));
	setBrush(pat);
}

void DiveCalculatedCeiling::recalc()
{
	dataModel->calculateDecompression();
}

void DiveCalculatedCeiling::settingsChanged()
{
	if (dataModel && is3mIncrement != prefs.calcceiling3m) {
		// recalculate that part.
		recalc();
	}
	is3mIncrement = prefs.calcceiling3m;
	setVisible(prefs.calcceiling);
}

void DiveReportedCeiling::settingsChanged()
{
	setVisible(prefs.dcceiling);
}

void DiveReportedCeiling::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	if (polygon().isEmpty())
		return;
	QGraphicsPolygonItem::paint(painter, option, widget);
}

void PartialPressureGasItem::modelDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
	//AbstractProfilePolygonItem::modelDataChanged();
	if (!shouldCalculateStuff(topLeft, bottomRight))
		return;

	plot_data *entry = dataModel->data().entry;
	QPolygonF poly;
	QPolygonF alertpoly;
	alertPolygons.clear();
	double threshold = 0.0;
	if (thresholdPtr)
		threshold = *thresholdPtr;
	bool inAlertFragment = false;
	for (int i = 0; i < dataModel->rowCount(); i++, entry++) {
		double value = dataModel->index(i, vDataColumn).data().toDouble();
		int time = dataModel->index(i, hDataColumn).data().toInt();
		QPointF point(hAxis->posAtValue(time), vAxis->posAtValue(value));
		poly.push_back(point);
		if (value >= threshold) {
			if (inAlertFragment) {
				alertPolygons.back().push_back(point);
			} else {
				alertpoly.clear();
				alertpoly.push_back(point);
				alertPolygons.append(alertpoly);
				inAlertFragment = true;
			}
		} else {
			inAlertFragment = false;
		}
	}
	setPolygon(poly);
	/*
	createPPLegend(trUtf8("pN" UTF8_SUBSCRIPT_2),getColor(PN2), legendPos);
	*/
}

void PartialPressureGasItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	const qreal pWidth = 0.0;
	painter->save();
	painter->setPen(QPen(normalColor, pWidth));
	painter->drawPolyline(polygon());

	QPolygonF poly;
	painter->setPen(QPen(alertColor, pWidth));
	Q_FOREACH (const QPolygonF &poly, alertPolygons)
		painter->drawPolyline(poly);
	painter->restore();
}

void PartialPressureGasItem::setThreshouldSettingsKey(double *prefPointer)
{
	thresholdPtr = prefPointer;
}

PartialPressureGasItem::PartialPressureGasItem() :
	thresholdPtr(NULL)
{
}

void PartialPressureGasItem::settingsChanged()
{
	QSettings s;
	s.beginGroup("TecDetails");
	setVisible(s.value(visibilityKey).toBool());
}

void PartialPressureGasItem::setVisibilitySettingsKey(const QString &key)
{
	visibilityKey = key;
}

void PartialPressureGasItem::setColors(const QColor &normal, const QColor &alert)
{
	normalColor = normal;
	alertColor = alert;
}
