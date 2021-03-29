// SPDX-License-Identifier: GPL-2.0
#include "profile-widget/diveprofileitem.h"
#include "qt-models/diveplotdatamodel.h"
#include "profile-widget/divecartesianaxis.h"
#include "profile-widget/divetextitem.h"
#include "profile-widget/animationfunctions.h"
#include "core/profile.h"
#include "qt-models/diveplannermodel.h"
#include "core/qthelper.h"
#include "core/settings/qPrefTechnicalDetails.h"
#include "core/settings/qPrefLog.h"
#include "libdivecomputer/parser.h"
#include "profile-widget/profilewidget2.h"

AbstractProfilePolygonItem::AbstractProfilePolygonItem(const DivePlotDataModel &model, const DiveCartesianAxis &horizontal, int hColumn,
						       const DiveCartesianAxis &vertical, int vColumn) :
	hAxis(horizontal), vAxis(vertical), dataModel(model), hDataColumn(hColumn), vDataColumn(vColumn)
{
	setCacheMode(DeviceCoordinateCache);
}

void AbstractProfilePolygonItem::clear()
{
	setPolygon(QPolygonF());
	qDeleteAll(texts);
	texts.clear();
}

void AbstractProfilePolygonItem::setVisible(bool visible)
{
	QGraphicsPolygonItem::setVisible(visible);
}

void AbstractProfilePolygonItem::replot(const dive *, bool)
{
	// Calculate the polygon. This is the polygon that will be painted on screen
	// on the ::paint method. Here we calculate the correct position of the points
	// regarting our cartesian plane ( made by the hAxis and vAxis ), the QPolygonF
	// is an array of QPointF's, so we basically get the point from the model, convert
	// to our coordinates, store. no painting is done here.
	QPolygonF poly;
	for (int i = 0, modelDataCount = dataModel.rowCount(); i < modelDataCount; i++) {
		qreal horizontalValue = dataModel.index(i, hDataColumn).data().toReal();
		qreal verticalValue = dataModel.index(i, vDataColumn).data().toReal();
		QPointF point(hAxis.posAtValue(horizontalValue), vAxis.posAtValue(verticalValue));
		poly.append(point);
	}
	setPolygon(poly);

	qDeleteAll(texts);
	texts.clear();
}

DiveProfileItem::DiveProfileItem(const DivePlotDataModel &model, const DiveCartesianAxis &hAxis, int hColumn, const DiveCartesianAxis &vAxis, int vColumn) :
	AbstractProfilePolygonItem(model, hAxis, hColumn, vAxis, vColumn),
	show_reported_ceiling(0), reported_ceiling_in_red(0)
{
}

void DiveProfileItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
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
	for (int i = 1, count = dataModel.rowCount(); i < count; i++) {
		QModelIndex colorIndex = dataModel.index(i, DivePlotDataModel::COLOR);
		pen.setBrush(QBrush(colorIndex.data(Qt::BackgroundRole).value<QColor>()));
		painter->setPen(pen);
		if (i < poly.count())
			painter->drawLine(poly[i - 1], poly[i]);
	}
	painter->restore();
}

void DiveProfileItem::replot(const dive *d, bool in_planner)
{
	AbstractProfilePolygonItem::replot(d, in_planner);
	if (polygon().isEmpty())
		return;

	show_reported_ceiling = prefs.dcceiling;
	reported_ceiling_in_red = prefs.redceiling;
	profileColor = dataModel.data().waypoint_above_ceiling ? QColor(Qt::red)
							       : getColor(DEPTH_BOTTOM);

	/* Show any ceiling we may have encountered */
	if (prefs.dcceiling && !prefs.redceiling) {
		QPolygonF p = polygon();
		plot_data *entry = dataModel.data().entry + dataModel.rowCount() - 1;
		for (int i = dataModel.rowCount() - 1; i >= 0; i--, entry--) {
			if (!entry->in_deco) {
				/* not in deco implies this is a safety stop, no ceiling */
				p.append(QPointF(hAxis.posAtValue(entry->sec), vAxis.posAtValue(0)));
			} else {
				p.append(QPointF(hAxis.posAtValue(entry->sec), vAxis.posAtValue(qMin(entry->stopdepth, entry->depth))));
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
	for (int i = 0, count = dataModel.rowCount(); i < count; i++) {
		struct plot_data *pd = dataModel.data().entry;
		struct plot_data *entry =  pd + i;
		// "min/max" are the 9-minute window min/max indices
		struct plot_data *min_entry = pd + entry->min;
		struct plot_data *max_entry = pd + entry->max;

		if (entry->depth < 2000)
			continue;

		if ((entry == max_entry) && entry->depth / 100 != last) {
			plot_depth_sample(entry, Qt::AlignHCenter | Qt::AlignBottom, getColor(SAMPLE_DEEP));
			last = entry->depth / 100;
		}

		if ((entry == min_entry) && entry->depth / 100 != last) {
			plot_depth_sample(entry, Qt::AlignHCenter | Qt::AlignTop, getColor(SAMPLE_SHALLOW));
			last = entry->depth / 100;
		}

		if (entry->depth != last)
			last = -1;
	}
}

void DiveProfileItem::plot_depth_sample(struct plot_data *entry, QFlags<Qt::AlignmentFlag> flags, const QColor &color)
{
	DiveTextItem *item = new DiveTextItem(this);
	item->setPos(hAxis.posAtValue(entry->sec), vAxis.posAtValue(entry->depth));
	item->setText(get_depth_string(entry->depth, true));
	item->setAlignment(flags);
	item->setBrush(color);
	texts.append(item);
}

DiveHeartrateItem::DiveHeartrateItem(const DivePlotDataModel &model, const DiveCartesianAxis &hAxis, int hColumn, const DiveCartesianAxis &vAxis, int vColumn) :
	AbstractProfilePolygonItem(model, hAxis, hColumn, vAxis, vColumn)
{
	QPen pen;
	pen.setBrush(QBrush(getColor(::HR_PLOT)));
	pen.setCosmetic(true);
	pen.setWidth(1);
	setPen(pen);
	connect(qPrefTechnicalDetails::instance(), &qPrefTechnicalDetails::hrgraphChanged, this, &DiveHeartrateItem::setVisible);
}

void DiveHeartrateItem::replot(const dive *, bool)
{
	int last = -300, last_printed_hr = 0, sec = 0;
	struct {
		int sec;
		int hr;
	} hist[3] = {};

	qDeleteAll(texts);
	texts.clear();
	// Ignore empty values. a heart rate of 0 would be a bad sign.
	QPolygonF poly;
	for (int i = 0, modelDataCount = dataModel.rowCount(); i < modelDataCount; i++) {
		int hr = dataModel.index(i, vDataColumn).data().toInt();
		if (!hr)
			continue;
		sec = dataModel.index(i, hDataColumn).data().toInt();
		QPointF point(hAxis.posAtValue(sec), vAxis.posAtValue(hr));
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
	text->setPos(QPointF(hAxis.posAtValue(sec), vAxis.posAtValue(hr)));
	text->setScale(0.7); // need to call this BEFORE setText()
	text->setText(QString("%1").arg(hr));
	texts.append(text);
}

void DiveHeartrateItem::paint(QPainter *painter, const QStyleOptionGraphicsItem*, QWidget*)
{
	if (polygon().isEmpty())
		return;
	painter->save();
	painter->setPen(pen());
	painter->drawPolyline(polygon());
	painter->restore();
}

DivePercentageItem::DivePercentageItem(const DivePlotDataModel &model, const DiveCartesianAxis &hAxis, int hColumn, const DiveCartesianAxis &vAxis, int vColumn, int i) :
	AbstractProfilePolygonItem(model, hAxis, hColumn, vAxis, vColumn),
	tissueIndex(i)
{
	connect(qPrefTechnicalDetails::instance(), &qPrefTechnicalDetails::percentagegraphChanged, this, &DivePercentageItem::setVisible);
}

void DivePercentageItem::replot(const dive *d, bool)
{
	// Ignore empty values. a heart rate of 0 would be a bad sign.
	QPolygonF poly;
	colors.clear();
	for (int i = 0, modelDataCount = dataModel.rowCount(); i < modelDataCount; i++) {
		int sec = dataModel.index(i, hDataColumn).data().toInt();
		QPointF point(hAxis.posAtValue(sec), vAxis.posAtValue(64 - 4 * tissueIndex));
		poly.append(point);

		double value = dataModel.index(i, vDataColumn).data().toDouble();
		struct gasmix gasmix = gasmix_air;
		const struct event *ev = NULL;
		gasmix = get_gasmix(d, get_dive_dc_const(d, dc_number), sec, &ev, gasmix);
		int inert = get_n2(gasmix) + get_he(gasmix);
		colors.push_back(ColorScale(value, inert));
	}
	setPolygon(poly);

	if (texts.count())
		texts.last()->setAlignment(Qt::AlignLeft | Qt::AlignBottom);
}

QColor DivePercentageItem::ColorScale(double value, int inert)
{
	QColor color;
	double scaledValue = value / (AMB_PERCENTAGE * inert) * 1000.0;
	if (scaledValue < 0.8)	// grade from cyan to blue to purple
		color.setHsvF(0.5 + 0.25 * scaledValue / 0.8, 1.0, 1.0);
	else if (scaledValue < 1.0)	// grade from magenta to black
		color.setHsvF(0.75, 1.0, (1.0 - scaledValue) / 0.2);
	else if (value < AMB_PERCENTAGE)	// grade from black to bright green
		color.setHsvF(0.333, 1.0, (value - AMB_PERCENTAGE * inert / 1000.0) / (AMB_PERCENTAGE - AMB_PERCENTAGE * inert / 1000.0));
	else if (value < 65)		// grade from bright green (0% M) to yellow-green (30% M)
		color.setHsvF(0.333 - 0.133 * (value - AMB_PERCENTAGE) / (65.0 - AMB_PERCENTAGE), 1.0, 1.0);
	else if (value < 85)		// grade from yellow-green (30% M) to orange (70% M)
		color.setHsvF(0.2 - 0.1 * (value - 65.0) / 20.0, 1.0, 1.0);
	else if (value < 100)		// grade from orange (70% M) to red (100% M)
		color.setHsvF(0.1 * (100.0 - value) / 15.0, 1.0, 1.0);
	else if (value < 120)		// M value exceeded - grade from red to white
		color.setHsvF(0.0, 1 - (value - 100.0) / 20.0, 1.0);
	else	// white
		color.setHsvF(0.0, 0.0, 1.0);
	return color;

}

void DivePercentageItem::paint(QPainter *painter, const QStyleOptionGraphicsItem*, QWidget*)
{
	if (polygon().isEmpty())
		return;
	painter->save();
	QPen mypen;
	mypen.setCapStyle(Qt::FlatCap);
	mypen.setCosmetic(false);
	QPolygonF poly = polygon();
	for (int i = 1; i < poly.count(); i++) {
		mypen.setBrush(QBrush(colors[i]));
		painter->setPen(mypen);
		painter->drawLine(poly[i - 1], poly[i]);
	}
	painter->restore();
}

DiveAmbPressureItem::DiveAmbPressureItem(const DivePlotDataModel &model, const DiveCartesianAxis &hAxis, int hColumn, const DiveCartesianAxis &vAxis, int vColumn) :
	AbstractProfilePolygonItem(model, hAxis, hColumn, vAxis, vColumn)
{
	QPen pen;
	pen.setBrush(QBrush(getColor(::AMB_PRESSURE_LINE)));
	pen.setCosmetic(true);
	pen.setWidth(2);
	setPen(pen);
}

void DiveAmbPressureItem::replot(const dive *, bool)
{
	int sec = 0;

	// Ignore empty values. a heart rate of 0 would be a bad sign.
	QPolygonF poly;
	for (int i = 0, modelDataCount = dataModel.rowCount(); i < modelDataCount; i++) {
		int hr = dataModel.index(i, vDataColumn).data().toInt();
		if (!hr)
			continue;
		sec = dataModel.index(i, hDataColumn).data().toInt();
		QPointF point(hAxis.posAtValue(sec), vAxis.posAtValue(hr));
		poly.append(point);
	}
	setPolygon(poly);

	if (texts.count())
		texts.last()->setAlignment(Qt::AlignLeft | Qt::AlignBottom);
}

void DiveAmbPressureItem::paint(QPainter *painter, const QStyleOptionGraphicsItem*, QWidget*)
{
	if (polygon().isEmpty())
		return;
	painter->save();
	painter->setPen(pen());
	painter->drawPolyline(polygon());
	painter->restore();
	connect(qPrefTechnicalDetails::instance(), &qPrefTechnicalDetails::percentagegraphChanged, this, &DiveAmbPressureItem::setVisible);
}

DiveGFLineItem::DiveGFLineItem(const DivePlotDataModel &model, const DiveCartesianAxis &hAxis, int hColumn, const DiveCartesianAxis &vAxis, int vColumn) :
	AbstractProfilePolygonItem(model, hAxis, hColumn, vAxis, vColumn)
{
	QPen pen;
	pen.setBrush(QBrush(getColor(::GF_LINE)));
	pen.setCosmetic(true);
	pen.setWidth(2);
	setPen(pen);
}

void DiveGFLineItem::replot(const dive *, bool)
{
	int sec = 0;

	// Ignore empty values. a heart rate of 0 would be a bad sign.
	QPolygonF poly;
	for (int i = 0, modelDataCount = dataModel.rowCount(); i < modelDataCount; i++) {
		int hr = dataModel.index(i, vDataColumn).data().toInt();
		if (!hr)
			continue;
		sec = dataModel.index(i, hDataColumn).data().toInt();
		QPointF point(hAxis.posAtValue(sec), vAxis.posAtValue(hr));
		poly.append(point);
	}
	setPolygon(poly);

	if (texts.count())
		texts.last()->setAlignment(Qt::AlignLeft | Qt::AlignBottom);
}

void DiveGFLineItem::paint(QPainter *painter, const QStyleOptionGraphicsItem*, QWidget*)
{
	if (polygon().isEmpty())
		return;
	painter->save();
	painter->setPen(pen());
	painter->drawPolyline(polygon());
	painter->restore();
	connect(qPrefTechnicalDetails::instance(), &qPrefTechnicalDetails::percentagegraphChanged, this, &DiveAmbPressureItem::setVisible);
}

DiveTemperatureItem::DiveTemperatureItem(const DivePlotDataModel &model, const DiveCartesianAxis &hAxis, int hColumn, const DiveCartesianAxis &vAxis, int vColumn) :
	AbstractProfilePolygonItem(model, hAxis, hColumn, vAxis, vColumn)
{
	QPen pen;
	pen.setBrush(QBrush(getColor(::TEMP_PLOT)));
	pen.setCosmetic(true);
	pen.setWidth(2);
	setPen(pen);
}

void DiveTemperatureItem::replot(const dive *, bool)
{
	int last = -300, last_printed_temp = 0, sec = 0, last_valid_temp = 0;

	qDeleteAll(texts);
	texts.clear();
	// Ignore empty values. things do not look good with '0' as temperature in kelvin...
	QPolygonF poly;
	for (int i = 0, modelDataCount = dataModel.rowCount(); i < modelDataCount; i++) {
		int mkelvin = dataModel.index(i, vDataColumn).data().toInt();
		if (!mkelvin)
			continue;
		last_valid_temp = mkelvin;
		sec = dataModel.index(i, hDataColumn).data().toInt();
		QPointF point(hAxis.posAtValue(sec), vAxis.posAtValue(mkelvin));
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
	temperature_t temp;
	temp.mkelvin = mkelvin;

	DiveTextItem *text = new DiveTextItem(this);
	text->setAlignment(Qt::AlignRight | Qt::AlignBottom);
	text->setBrush(getColor(TEMP_TEXT));
	text->setPos(QPointF(hAxis.posAtValue(sec), vAxis.posAtValue(mkelvin)));
	text->setScale(0.8); // need to call this BEFORE setText()
	text->setText(get_temperature_string(temp, true));
	texts.append(text);
}

void DiveTemperatureItem::paint(QPainter *painter, const QStyleOptionGraphicsItem*, QWidget*)
{
	if (polygon().isEmpty())
		return;
	painter->save();
	painter->setPen(pen());
	painter->drawPolyline(polygon());
	painter->restore();
}

DiveMeanDepthItem::DiveMeanDepthItem(const DivePlotDataModel &model, const DiveCartesianAxis &hAxis, int hColumn, const DiveCartesianAxis &vAxis, int vColumn) :
	AbstractProfilePolygonItem(model, hAxis, hColumn, vAxis, vColumn)
{
	QPen pen;
	pen.setBrush(QBrush(getColor(::HR_AXIS)));
	pen.setCosmetic(true);
	pen.setWidth(2);
	setPen(pen);
	lastRunningSum = 0.0;
}

void DiveMeanDepthItem::replot(const dive *, bool)
{
	double meandepthvalue = 0.0;

	QPolygonF poly;
	plot_data *entry = dataModel.data().entry;
	for (int i = 0, modelDataCount = dataModel.rowCount(); i < modelDataCount; i++, entry++) {
		// Ignore empty values
		if (entry->running_sum == 0 || entry->sec == 0)
			continue;

		meandepthvalue = entry->running_sum / entry->sec;
		QPointF point(hAxis.posAtValue(entry->sec), vAxis.posAtValue(meandepthvalue));
		poly.append(point);
	}
	lastRunningSum = meandepthvalue;
	setPolygon(poly);
	createTextItem();
}


void DiveMeanDepthItem::paint(QPainter *painter, const QStyleOptionGraphicsItem*, QWidget*)
{
	if (polygon().isEmpty())
		return;
	painter->save();
	painter->setPen(pen());
	painter->drawPolyline(polygon());
	painter->restore();
	connect(qPrefLog::instance(), &qPrefLog::show_average_depthChanged, this, &DiveAmbPressureItem::setVisible);
}

void DiveMeanDepthItem::createTextItem()
{
	plot_data *entry = dataModel.data().entry;
	int sec = entry[dataModel.rowCount()-1].sec;
	qDeleteAll(texts);
	texts.clear();
	DiveTextItem *text = new DiveTextItem(this);
	text->setAlignment(Qt::AlignRight | Qt::AlignTop);
	text->setBrush(getColor(TEMP_TEXT));
	text->setPos(QPointF(hAxis.posAtValue(sec) + 1, vAxis.posAtValue(lastRunningSum)));
	text->setScale(0.8); // need to call this BEFORE setText()
	text->setText(get_depth_string(lrint(lastRunningSum), true));
	texts.append(text);
}

void DiveGasPressureItem::replot(const dive *d, bool in_planner)
{
	const struct plot_info *pInfo = &dataModel.data();
	std::vector<int> plotted_cyl(pInfo->nr_cylinders, false);
	std::vector<int> last_plotted(pInfo->nr_cylinders, 0);
	std::vector<std::vector<Entry>> poly(pInfo->nr_cylinders);
	QPolygonF boundingPoly;
	polygons.clear();

	for (int i = 0, count = dataModel.rowCount(); i < count; i++) {
		const struct plot_data *entry = pInfo->entry + i;

		for (int cyl = 0; cyl < pInfo->nr_cylinders; cyl++) {
			int mbar = get_plot_pressure(pInfo, i, cyl);
			int time = entry->sec;

			if (!mbar)
				continue;

			QPointF point(hAxis.posAtValue(time), vAxis.posAtValue(mbar));
			boundingPoly.push_back(point);

			QColor color;
			if (!in_planner) {
				if (entry->sac)
					color = getSacColor(entry->sac, d->sac);
				else
					color = MED_GRAY_HIGH_TRANS;
			} else {
				if (mbar < 0)
					color = MAGENTA;
				else
					color = getPressureColor(entry->density);
			}

			if (plotted_cyl[cyl]) {
				/* Have we used this cylinder in the last two minutes? Continue */
				if (time - last_plotted[cyl] <= 2*60) {
					poly[cyl].push_back({ point, color });
					last_plotted[cyl] = time;
					continue;
				}

				/* Finish the previous one, start a new one */
				polygons.push_back(std::move(poly[cyl]));
				poly[cyl].clear();
			}

			plotted_cyl[cyl] = true;
			last_plotted[cyl] = time;
			poly[cyl].push_back({ point, color });
		}
	}

	for (int cyl = 0; cyl < pInfo->nr_cylinders; cyl++) {
		if (!plotted_cyl[cyl])
			continue;
		polygons.push_back(poly[cyl]);
	}

	setPolygon(boundingPoly);
	qDeleteAll(texts);
	texts.clear();

	std::vector<int> seen_cyl(pInfo->nr_cylinders, false);
	std::vector<int> last_pressure(pInfo->nr_cylinders, 0);
	std::vector<int> last_time(pInfo->nr_cylinders, 0);

	// These are offset values used to print the gas lables and pressures on a
	// dive profile at appropriate Y-coordinates. We alternate aligning the
	// label and the gas pressure above and under the pressure line.
	// The values are historical, and we could try to pick the over/under
	// depending on whether this pressure is higher or lower than the average.
	// Right now it's just strictly alternating when you have multiple gas
	// pressures.

	QFlags<Qt::AlignmentFlag> alignVar = Qt::AlignTop;
	std::vector<QFlags<Qt::AlignmentFlag>> align(pInfo->nr_cylinders);

	double axisRange = (vAxis.maximum() - vAxis.minimum())/1000;	// Convert axis pressure range to bar
	double axisLog = log10(log10(axisRange));

	for (int i = 0, count = dataModel.rowCount(); i < count; i++) {
		const struct plot_data *entry = pInfo->entry + i;

		for (int cyl = 0; cyl < pInfo->nr_cylinders; cyl++) {
			int mbar = get_plot_pressure(pInfo, i, cyl);

			if (!mbar)
				continue;

			if (!seen_cyl[cyl]) {
				double value_y_offset, label_y_offset;

				// Magic Y offset depending on whether we're aliging
				// the top of the text or the bottom of the text to
				// the pressure line.
				value_y_offset = -0.5;
				if (alignVar & Qt::AlignTop) {
					label_y_offset = 5 * axisLog;
				} else {
					label_y_offset = -7 * axisLog;
				}
				plotPressureValue(mbar, entry->sec, alignVar, value_y_offset);
				plotGasValue(mbar, entry->sec, get_cylinder(d, cyl)->gasmix, alignVar, label_y_offset);
				seen_cyl[cyl] = true;

				/* Alternate alignment as we see cylinder use.. */
				align[cyl] = alignVar;
				alignVar ^= Qt::AlignTop | Qt::AlignBottom;
			}
			last_pressure[cyl] = mbar;
			last_time[cyl] = entry->sec;
		}
	}

	// For each cylinder, on right hand side of profile, write cylinder pressure
	for (int cyl = 0; cyl < pInfo->nr_cylinders; cyl++) {
		if (last_time[cyl]) {
			double value_y_offset = -0.5;
			plotPressureValue(last_pressure[cyl], last_time[cyl], align[cyl] | Qt::AlignLeft, value_y_offset);
		}
	}
}

void DiveGasPressureItem::plotPressureValue(int mbar, int sec, QFlags<Qt::AlignmentFlag> align, double pressure_offset)
{
	const char *unit;
	int pressure = get_pressure_units(mbar, &unit);
	DiveTextItem *text = new DiveTextItem(this);
	text->setPos(hAxis.posAtValue(sec), vAxis.posAtValue(mbar) + pressure_offset );
	text->setText(QString("%1%2").arg(pressure).arg(unit));
	text->setAlignment(align);
	text->setBrush(getColor(PRESSURE_TEXT));
	texts.push_back(text);
}

void DiveGasPressureItem::plotGasValue(int mbar, int sec, struct gasmix gasmix, QFlags<Qt::AlignmentFlag> align, double gasname_offset)
{
	QString gas = get_gas_string(gasmix);
	DiveTextItem *text = new DiveTextItem(this);
	text->setPos(hAxis.posAtValue(sec), vAxis.posAtValue(mbar) + gasname_offset );
	text->setText(gas);
	text->setAlignment(align);
	text->setBrush(getColor(PRESSURE_TEXT));
	texts.push_back(text);
}

void DiveGasPressureItem::paint(QPainter *painter, const QStyleOptionGraphicsItem*, QWidget*)
{
	if (polygon().isEmpty())
		return;
	QPen pen;
	pen.setCosmetic(true);
	pen.setWidth(2);
	painter->save();
	for (const std::vector<Entry> &poly: polygons) {
		for (size_t i = 1; i < poly.size(); i++) {
			pen.setBrush(poly[i].col);
			painter->setPen(pen);
			painter->drawLine(poly[i - 1].pos, poly[i].pos);
		}
	}
	painter->restore();
}

DiveCalculatedCeiling::DiveCalculatedCeiling(const DivePlotDataModel &model, const DiveCartesianAxis &hAxis, int hColumn, const DiveCartesianAxis &vAxis, int vColumn, ProfileWidget2 *widget) :
	AbstractProfilePolygonItem(model, hAxis, hColumn, vAxis, vColumn),
	profileWidget(widget)
{
	connect(qPrefTechnicalDetails::instance(), &qPrefTechnicalDetails::calcceilingChanged, this, &DiveCalculatedCeiling::setVisible);
	setVisible(prefs.calcceiling);
}

void DiveCalculatedCeiling::replot(const dive *d, bool in_planner)
{
	AbstractProfilePolygonItem::replot(d, in_planner);
	// Add 2 points to close the polygon.
	QPolygonF poly = polygon();
	if (poly.isEmpty())
		return;
	QPointF p1 = poly.first();
	QPointF p2 = poly.last();

	poly.prepend(QPointF(p1.x(), vAxis.posAtValue(0)));
	poly.append(QPointF(p2.x(), vAxis.posAtValue(0)));
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

DiveCalculatedTissue::DiveCalculatedTissue(const DivePlotDataModel &model, const DiveCartesianAxis &hAxis, int hColumn, const DiveCartesianAxis &vAxis, int vColumn, ProfileWidget2 *widget) :
	DiveCalculatedCeiling(model, hAxis, hColumn, vAxis, vColumn, widget)
{
	setVisible(true);
	connect(qPrefTechnicalDetails::instance(), &qPrefTechnicalDetails::calcalltissuesChanged, this, &DiveCalculatedTissue::setVisible);
	connect(qPrefTechnicalDetails::instance(), &qPrefTechnicalDetails::calcceilingChanged, this, &DiveCalculatedTissue::setVisible);
}

void DiveCalculatedTissue::setVisible(bool)
{
	DiveCalculatedCeiling::setVisible(prefs.calcalltissues && prefs.calcceiling);
}

DiveReportedCeiling::DiveReportedCeiling(const DivePlotDataModel &model, const DiveCartesianAxis &hAxis, int hColumn, const DiveCartesianAxis &vAxis, int vColumn) :
	AbstractProfilePolygonItem(model, hAxis, hColumn, vAxis, vColumn)
{
	connect(qPrefTechnicalDetails::instance(), &qPrefTechnicalDetails::dcceilingChanged, this, &DiveReportedCeiling::setVisible);
	setVisible(prefs.dcceiling);
}

void DiveReportedCeiling::replot(const dive *, bool)
{
	QPolygonF p;
	p.append(QPointF(hAxis.posAtValue(0), vAxis.posAtValue(0)));
	plot_data *entry = dataModel.data().entry;
	for (int i = 0, count = dataModel.rowCount(); i < count; i++, entry++) {
		if (entry->in_deco && entry->stopdepth) {
			p.append(QPointF(hAxis.posAtValue(entry->sec), vAxis.posAtValue(qMin(entry->stopdepth, entry->depth))));
		} else {
			p.append(QPointF(hAxis.posAtValue(entry->sec), vAxis.posAtValue(0)));
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

void DiveReportedCeiling::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	if (polygon().isEmpty())
		return;
	QGraphicsPolygonItem::paint(painter, option, widget);
}

void PartialPressureGasItem::replot(const dive *, bool)
{
	plot_data *entry = dataModel.data().entry;
	QPolygonF poly;
	QPolygonF alertpoly;
	alertPolygons.clear();
	double threshold_min = 100.0; // yes, a ridiculous high partial pressure
	double threshold_max = 0.0;
	if (thresholdPtrMax)
		threshold_max = *thresholdPtrMax;
	if (thresholdPtrMin)
		threshold_min = *thresholdPtrMin;
	bool inAlertFragment = false;
	for (int i = 0; i < dataModel.rowCount(); i++, entry++) {
		double value = dataModel.index(i, vDataColumn).data().toDouble();
		int time = dataModel.index(i, hDataColumn).data().toInt();
		QPointF point(hAxis.posAtValue(time), vAxis.posAtValue(value));
		poly.push_back(point);
		if (thresholdPtrMax && value >= threshold_max) {
			if (inAlertFragment) {
				alertPolygons.back().push_back(point);
			} else {
				alertpoly.clear();
				alertpoly.push_back(point);
				alertPolygons.append(alertpoly);
				inAlertFragment = true;
			}
		} else if (thresholdPtrMin && value <= threshold_min) {
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
	createPPLegend(trUtf8("pN₂"), getColor(PN2), legendPos);
	*/
}

void PartialPressureGasItem::paint(QPainter *painter, const QStyleOptionGraphicsItem*, QWidget*)
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

void PartialPressureGasItem::setThresholdSettingsKey(const double *prefPointerMin, const double *prefPointerMax)
{
	thresholdPtrMin = prefPointerMin;
	thresholdPtrMax = prefPointerMax;
}

PartialPressureGasItem::PartialPressureGasItem(const DivePlotDataModel &model, const DiveCartesianAxis &hAxis, int hColumn, const DiveCartesianAxis &vAxis, int vColumn) :
	AbstractProfilePolygonItem(model, hAxis, hColumn, vAxis, vColumn),
	thresholdPtrMin(NULL),
	thresholdPtrMax(NULL)
{
}

void PartialPressureGasItem::setColors(const QColor &normal, const QColor &alert)
{
	normalColor = normal;
	alertColor = alert;
}
