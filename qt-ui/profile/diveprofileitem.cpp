#include "diveprofileitem.h"
#include "diveplotdatamodel.h"
#include "divecartesianaxis.h"
#include "graphicsview-common.h"
#include "divetextitem.h"
#include "profile.h"
#include "dive.h"
#include "profilegraphics.h"
#include "preferences.h"
#include "helpers.h"

#include <QPen>
#include <QPainter>
#include <QLinearGradient>
#include <QDebug>
#include <QApplication>
#include <QGraphicsItem>
#include <QSettings>

AbstractProfilePolygonItem::AbstractProfilePolygonItem(): QObject(), QGraphicsPolygonItem(),
	hAxis(NULL), vAxis(NULL), dataModel(NULL), hDataColumn(-1), vDataColumn(-1)
{
	connect(PreferencesDialog::instance(), SIGNAL(settingsChanged()), this, SLOT(preferencesChanged()));
}

void AbstractProfilePolygonItem::preferencesChanged()
{
}

void AbstractProfilePolygonItem::setHorizontalAxis(DiveCartesianAxis* horizontal)
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

void AbstractProfilePolygonItem::setModel(DivePlotDataModel* model)
{
	dataModel = model;
	connect(dataModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(modelDataChanged(QModelIndex, QModelIndex)));
	connect(dataModel, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)), this, SLOT(modelDataRemoved(QModelIndex, int, int)));
	modelDataChanged();
}

void AbstractProfilePolygonItem::modelDataRemoved(const QModelIndex& parent, int from, int to)
{
	setPolygon(QPolygonF());
	qDeleteAll(texts);
	texts.clear();
}

void AbstractProfilePolygonItem::setVerticalAxis(DiveCartesianAxis* vertical)
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

bool AbstractProfilePolygonItem::shouldCalculateStuff(const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
	if (!hAxis || !vAxis)
		return false;
	if (!dataModel || dataModel->rowCount() == 0)
		return false;
	if (hDataColumn == -1 || vDataColumn == -1)
		return false;
	if ( topLeft.isValid() && bottomRight.isValid()){
		if ((topLeft.column() >= vDataColumn || topLeft.column() >= hDataColumn ) &&
		(bottomRight.column() <= vDataColumn || topLeft.column() <= hDataColumn )){
			return true;
		}
	}
	return true;
}

void AbstractProfilePolygonItem::modelDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight)
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
		QPointF point( hAxis->posAtValue(horizontalValue), vAxis->posAtValue(verticalValue));
		poly.append(point);
	}
	setPolygon(poly);

	qDeleteAll(texts);
	texts.clear();
}

DiveProfileItem::DiveProfileItem() : show_reported_ceiling(0), reported_ceiling_in_red(0)
{

}

void DiveProfileItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
	Q_UNUSED(widget);
	if(polygon().isEmpty())
		return;

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
		painter->drawLine(poly[i-1],poly[i]);
	}
}

void DiveProfileItem::modelDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
	if(!shouldCalculateStuff(topLeft, bottomRight))
		return;

	AbstractProfilePolygonItem::modelDataChanged(topLeft, bottomRight);
	if (polygon().isEmpty())
		return;

	show_reported_ceiling = prefs.profile_dc_ceiling;
	reported_ceiling_in_red = prefs.profile_red_ceiling;

	/* Show any ceiling we may have encountered */
	if (prefs.profile_dc_ceiling && !prefs.profile_red_ceiling) {
		QPolygonF p = polygon();
		plot_data *entry = dataModel->data().entry + dataModel->rowCount()-1;
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

		struct plot_data *entry = dataModel->data().entry+i;
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

void DiveProfileItem::preferencesChanged()
{
	//TODO: Only modelDataChanged() here if we need to rebuild the graph ( for instance,
	// if the prefs.profile_dc_ceiling are enabled, but prefs.profile_red_ceiling is disabled
	// and only if it changed something. let's not waste cpu cycles repoloting something we don't need to.
		modelDataChanged();
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

DiveHeartrateItem::DiveHeartrateItem()
{
	QPen pen;
	pen.setBrush(QBrush(getColor(::HR_PLOT)));
	pen.setCosmetic(true);
	pen.setWidth(1);
	setPen(pen);
}

void DiveHeartrateItem::modelDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
	int last = -300, last_printed_hr = 0, sec = 0;
	struct {
		int sec;
		int hr;
	} hist[3] = {0};

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
		QPointF point( hAxis->posAtValue(sec), vAxis->posAtValue(hr));
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

	if( texts.count())
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

void DiveHeartrateItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	if(polygon().isEmpty())
		return;
	painter->setPen(pen());
	painter->drawPolyline(polygon());
}

DiveTemperatureItem::DiveTemperatureItem()
{
	QPen pen;
	pen.setBrush(QBrush(getColor(::TEMP_PLOT)));
	pen.setCosmetic(true);
	pen.setWidth(2);
	setPen(pen);
}

void DiveTemperatureItem::modelDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight)
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
	if (last_valid_temp > 200000 &&
	    ((abs(last_valid_temp - last_printed_temp) > 500) || ((double)last / (double)sec < 0.75))) {
		createTextItem(sec, last_valid_temp);
	}
	if( texts.count())
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

void DiveTemperatureItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	if(polygon().isEmpty())
		return;
	painter->setPen(pen());
	painter->drawPolyline(polygon());
}

void DiveGasPressureItem::modelDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
	// We don't have enougth data to calculate things, quit.
	if (!shouldCalculateStuff(topLeft, bottomRight))
		return;
	int last_index = -1;
	QPolygonF boundingPoly; // This is the "Whole Item", but a pressure can be divided in N Polygons.
	polygons.clear();

	for (int i = 0, count = dataModel->rowCount(); i < count; i++) {
		plot_data* entry = dataModel->data().entry + i;
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
	qDeleteAll(texts);
	texts.clear();
	int mbar, cyl;
	int seen_cyl[MAX_CYLINDERS] = { false, };
	int last_pressure[MAX_CYLINDERS] = { 0, };
	int last_time[MAX_CYLINDERS] = { 0, };
	struct plot_data *entry;
	struct dive *dive = getDiveById(dataModel->id());
	Q_ASSERT(dive != NULL);

	cyl = -1;
	for (int i = 0, count = dataModel->rowCount(); i < count; i++) {
		entry = dataModel->data().entry + i;
		mbar = GET_PRESSURE(entry);

		if (!mbar)
			continue;
		if (cyl != entry->cylinderindex) {
			cyl = entry->cylinderindex;
			if (!seen_cyl[cyl]) {
				plot_pressure_value(mbar, entry->sec, Qt::AlignRight | Qt::AlignTop);
				plot_gas_value(mbar, entry->sec, Qt::AlignRight | Qt::AlignBottom,
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
			plot_pressure_value(last_pressure[cyl], last_time[cyl], Qt::AlignLeft | Qt::AlignTop);
		}
	}
}

void DiveGasPressureItem::plot_pressure_value(int mbar, int sec, QFlags<Qt::AlignmentFlag> flags)
{
	const char *unit;
	int pressure = get_pressure_units(mbar, &unit);
	DiveTextItem *text = new DiveTextItem(this);
	text->setPos(hAxis->posAtValue(sec), vAxis->posAtValue(mbar)-0.5);
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
	if(polygon().isEmpty())
		return;
	QPen pen;
	pen.setCosmetic(true);
	pen.setWidth(2);
	struct dive *d = getDiveById(dataModel->id());
	if (!d)
		return;
	struct plot_data *entry = dataModel->data().entry;
	Q_FOREACH(const QPolygonF& poly, polygons) {
		for (int i = 1, count = poly.count(); i < count; i++, entry++) {
			pen.setBrush(getSacColor(entry->sac, d->sac));
			painter->setPen(pen);
			painter->drawLine(poly[i-1],poly[i]);
		}
	}
}

DiveCalculatedCeiling::DiveCalculatedCeiling() : is3mIncrement(false), gradientFactor(new DiveTextItem(this))
{
	gradientFactor->setY(0);
	gradientFactor->setBrush(getColor(PRESSURE_TEXT));
	gradientFactor->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
	preferencesChanged();
}

void DiveCalculatedCeiling::modelDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
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
	setPen(QPen(QBrush(Qt::NoBrush),0));
	setBrush(pat);

	gradientFactor->setX(poly.boundingRect().width()/2 + poly.boundingRect().x());
	gradientFactor->setText(QString("GF %1/%2").arg(prefs.gflow).arg(prefs.gfhigh));
}

void DiveCalculatedCeiling::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	if(polygon().isEmpty())
		return;
	QGraphicsPolygonItem::paint(painter, option, widget);
}

DiveCalculatedTissue::DiveCalculatedTissue()
{
	preferencesChanged();
}

void DiveCalculatedTissue::preferencesChanged()
{
	QSettings s;
	s.beginGroup("TecDetails");
	setVisible(s.value("calcalltissues").toBool() && s.value("calcceiling").toBool());
}

void DiveReportedCeiling::modelDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
	if(!shouldCalculateStuff(topLeft, bottomRight))
		return;

	QPolygonF p;
	p.append(QPointF(hAxis->posAtValue(0), vAxis->posAtValue(0)));
	plot_data *entry = dataModel->data().entry;
	for (int i = 0, count = dataModel->rowCount(); i < count; i++, entry++) {
		if (entry->in_deco && entry->stopdepth) {
			if (entry->stopdepth < entry->depth) {
				p.append(QPointF(hAxis->posAtValue(entry->sec), vAxis->posAtValue(entry->stopdepth)));
			} else {
				p.append(QPointF(hAxis->posAtValue(entry->sec), vAxis->posAtValue(entry->depth)));
			}
		} else {
			p.append(QPointF(hAxis->posAtValue(entry->sec), vAxis->posAtValue(0)));
		}
	}
	setPolygon(p);
	QLinearGradient pat(0, p.boundingRect().top(), 0, p.boundingRect().bottom());
	pat.setColorAt(0, getColor(CEILING_SHALLOW));
	pat.setColorAt(1, getColor(CEILING_DEEP));
	setPen(QPen(QBrush(Qt::NoBrush),0));
	setBrush(pat);
}

void DiveCalculatedCeiling::preferencesChanged()
{
	QSettings s;
	s.beginGroup("TecDetails");

	bool shouldShow3mIncrement = s.value("calcceiling3m").toBool();
	if ( dataModel && is3mIncrement != shouldShow3mIncrement){
		// recalculate that part.
		dataModel->calculateDecompression();
	}
	is3mIncrement = shouldShow3mIncrement;
	setVisible(s.value("calcceiling").toBool());
}

void DiveReportedCeiling::preferencesChanged()
{
	QSettings s;
	s.beginGroup("TecDetails");
	setVisible(s.value("redceiling").toBool());
}

void DiveReportedCeiling::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	if(polygon().isEmpty())
		return;
	QGraphicsPolygonItem::paint(painter, option, widget);
}

MeanDepthLine::MeanDepthLine() : meanDepth(0), leftText(new DiveTextItem(this)), rightText(new DiveTextItem(this))
{
	leftText->setAlignment(Qt::AlignRight | Qt::AlignBottom);
	leftText->setBrush(getColor(MEAN_DEPTH));
	rightText->setAlignment(Qt::AlignLeft | Qt::AlignBottom);
	rightText->setBrush(getColor(MEAN_DEPTH));
	leftText->setPos(0, 0);
	rightText->setPos(line().length(), 0);
}

void MeanDepthLine::setLine(qreal x1, qreal y1, qreal x2, qreal y2)
{
	QGraphicsLineItem::setLine(x1, y1, x2, y2);
	leftText->setPos(x1, 0);
	rightText->setPos(x2, 0);
}

void MeanDepthLine::setMeanDepth(int value)
{
	leftText->setText(get_depth_string(value, false, true));
	rightText->setText(get_depth_string(value, false, true));
	meanDepth = value;
}

void MeanDepthLine::setAxis(DiveCartesianAxis* a)
{
	connect(a, SIGNAL(sizeChanged()), this, SLOT(axisLineChanged()));
}

void MeanDepthLine::axisLineChanged()
{
	DiveCartesianAxis *axis = qobject_cast<DiveCartesianAxis*>(sender());
	animateMoveTo(x(),axis->posAtValue(meanDepth));
}

void PartialPressureGasItem::modelDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
	//AbstractProfilePolygonItem::modelDataChanged();
	if (!shouldCalculateStuff(topLeft, bottomRight))
		return;

	plot_data *entry = dataModel->data().entry;
	QPolygonF poly;
	alertPoly.clear();
	QSettings s;
	s.beginGroup("TecDetails");
	double threshould = s.value(threshouldKey).toDouble();
	for(int i = 0;  i < dataModel->rowCount(); i++, entry++){
		double value = dataModel->index(i, vDataColumn).data().toDouble();
		int time = dataModel->index(i, hDataColumn).data().toInt();
		QPointF point(hAxis->posAtValue(time), vAxis->posAtValue(value));
		poly.push_back( point );
		if (value >= threshould)
			alertPoly.push_back(point);
	}
	setPolygon(poly);
	/*
	createPPLegend(trUtf8("pN" UTF8_SUBSCRIPT_2),getColor(PN2), legendPos);
	*/
}

void PartialPressureGasItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	painter->setPen(normalColor);
	painter->drawPolyline(polygon());
	painter->setPen(alertColor);
	painter->drawPolyline(alertPoly);
}

void PartialPressureGasItem::setThreshouldSettingsKey(const QString& threshouldSettingsKey)
{
	threshouldKey = threshouldSettingsKey;
}

PartialPressureGasItem::PartialPressureGasItem()
{
}

void PartialPressureGasItem::preferencesChanged()
{
	QSettings s;
	s.beginGroup("TecDetails");
	setVisible( s.value(visibilityKey).toBool() );
}

void PartialPressureGasItem::setVisibilitySettingsKey(const QString& key)
{
	visibilityKey = key;
}

void PartialPressureGasItem::setColors(const QColor& normal, const QColor& alert)
{
	normalColor = normal;
	alertColor = alert;
}
