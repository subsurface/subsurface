// SPDX-License-Identifier: GPL-2.0
#include "profile-widget/diveprofileitem.h"
#include "profile-widget/divecartesianaxis.h"
#include "profile-widget/divetextitem.h"
#include "profile-widget/animationfunctions.h"
#include "core/profile.h"
#include "qt-models/diveplannermodel.h"
#include "core/qthelper.h"
#include "core/settings/qPrefTechnicalDetails.h"
#include "core/settings/qPrefLog.h"
#include "libdivecomputer/parser.h"

#include <QPainter>

AbstractProfilePolygonItem::AbstractProfilePolygonItem(const plot_info &pInfo, const DiveCartesianAxis &horizontal,
						       const DiveCartesianAxis &vertical, DataAccessor accessor,
						       double dpr) :
	hAxis(horizontal), vAxis(vertical), pInfo(pInfo), accessor(accessor), dpr(dpr), from(0), to(0)
{
	setCacheMode(DeviceCoordinateCache);
}

AbstractProfilePolygonItem::~AbstractProfilePolygonItem()
{
}

void AbstractProfilePolygonItem::clear()
{
	setPolygon(QPolygonF());
	texts.clear();
}

static std::pair<double,double> clip(double x1, double y1, double x2, double y2, double x)
{
	double rel = fabs(x2 - x1) > 1e-10 ? (x - x1) / (x2 - x1) : 0.5;
	return { x, (y2 - y1) * rel + y1 };
}

void AbstractProfilePolygonItem::clipStart(double &x, double &y, double next_x, double next_y) const
{
	if (x < hAxis.minimum())
		std::tie(x, y) = clip(x, y, next_x, next_y, hAxis.minimum());
}

void AbstractProfilePolygonItem::clipStop(double &x, double &y, double prev_x, double prev_y) const
{
	if (x > hAxis.maximum())
		std::tie(x, y) = clip(prev_x, prev_y, x, y, hAxis.maximum());
}

std::pair<double, double> AbstractProfilePolygonItem::getPoint(int i) const
{
	const struct plot_data *data = pInfo.entry;
	double x = data[i].sec;
	double y = accessor(data[i]);

	// Do clipping of first and last value
	if (i == from && i < to) {
		double next_x = data[i+1].sec;
		double next_y = accessor(data[i+1]);
		clipStart(x, y, next_x, next_y);
	}
	if (i == to - 1 && i > 0) {
		double prev_x = data[i-1].sec;
		double prev_y = accessor(data[i-1]);
		clipStop(x, y, prev_x, prev_y);
	}

	return { x, y };
}

void AbstractProfilePolygonItem::makePolygon(int fromIn, int toIn)
{
	from = fromIn;
	to = toIn;

	// Calculate the polygon. This is the polygon that will be painted on screen
	// on the ::paint method. Here we calculate the correct position of the points
	// regarting our cartesian plane ( made by the hAxis and vAxis ), the QPolygonF
	// is an array of QPointF's, so we basically get the point from the model, convert
	// to our coordinates, store. no painting is done here.
	QPolygonF poly;
	for (int i = from; i < to; i++) {
		auto [horizontalValue, verticalValue] = getPoint(i);

		if (i == from) {
			QPointF point(hAxis.posAtValue(horizontalValue), vAxis.posAtValue(0.0));
			poly.append(point);
		}
		QPointF point(hAxis.posAtValue(horizontalValue), vAxis.posAtValue(verticalValue));
		poly.append(point);
		if (i == to - 1) {
			QPointF point(hAxis.posAtValue(horizontalValue), vAxis.posAtValue(0.0));
			poly.append(point);
		}
	}
	setPolygon(poly);

	texts.clear();
}

DiveProfileItem::DiveProfileItem(const plot_info &pInfo, const DiveCartesianAxis &hAxis,
				 const DiveCartesianAxis &vAxis, DataAccessor accessor, double dpr) :
	AbstractProfilePolygonItem(pInfo, hAxis, vAxis, accessor, dpr)
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
	const struct plot_data *data = pInfo.entry;
	// This paints the colors of the velocities.
	for (int i = from + 1; i < to; i++) {
		QColor color = getColor((color_index_t)(VELOCITY_COLORS_START_IDX + data[i].velocity));
		pen.setBrush(QBrush(color));
		painter->setPen(pen);
		if (i - from < poly.count() - 1)
			painter->drawLine(poly[i - from], poly[i - from + 1]);
	}
	painter->restore();
}

static bool comp_depth(const struct plot_data &p1, const struct plot_data &p2)
{
	return p1.depth < p2.depth;
}

void DiveProfileItem::replot(const dive *d, int from, int to, bool in_planner)
{
	makePolygon(from, to);
	if (polygon().isEmpty())
		return;

	profileColor = pInfo.waypoint_above_ceiling ? QColor(Qt::red)
						    : getColor(DEPTH_BOTTOM);

	/* Show any ceiling we may have encountered */
	if (prefs.dcceiling && !prefs.redceiling) {
		QPolygonF p = polygon();
		plot_data *entry = pInfo.entry + to - 1;
		for (int i = to - 1; i >= from; i--, entry--) {
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

	// No point in searching peaks with less than three samples
	if (to - from < 3)
		return;

	const int half_interval = vAxis.getMinLabelDistance(hAxis);
	const int min_depth = 2000; // in mm
	const int min_prominence = 2000; // in mm (should this adapt to depth range?)
	const plot_data *data = pInfo.entry;
	const int max_peaks = (data[to - 1].sec - data[from].sec) / half_interval + 1;
	struct Peak {
		int range_from;
		int range_to;
		int peak;
	};
	std::vector<Peak> stack;
	stack.reserve(max_peaks);
	int highest_peak = std::max_element(data + from, data + to, comp_depth) - data;
	if (data[highest_peak].depth < min_depth)
		return;
	stack.push_back(Peak{ from, to, highest_peak });
	while (!stack.empty()) {
		Peak act_peak = stack.back();
		stack.pop_back();
		plot_depth_sample(data[act_peak.peak], Qt::AlignHCenter | Qt::AlignTop, getColor(SAMPLE_DEEP));

		// Skip half_interval seconds to the left and right of peak
		// and add new peaks if there is enough place.
		const plot_data &act_sample = data[act_peak.peak];
		int valley = act_peak.peak;

		// Search for first sample outside minimum range to the right.
		int new_from;
		for (new_from = act_peak.peak + 1; new_from + 3 < act_peak.range_to; ++new_from) {
			if (data[new_from].sec > act_sample.sec + half_interval)
				break;
			if (data[new_from].depth < data[valley].depth)
				valley = new_from;
		}
		// Continue search until peaks reach the minimum prominence (height from valley).
		for ( ; new_from + 3 < act_peak.range_to; ++new_from) {
			if (data[new_from].depth >= data[valley].depth + min_prominence) {
				int new_peak = std::max_element(data + new_from, data + act_peak.range_to, comp_depth) - data;
				if (data[new_peak].depth < min_depth)
					break;
				stack.push_back(Peak{ new_from, act_peak.range_to, new_peak });

				if (data[valley].depth >= min_depth)
					plot_depth_sample(data[valley], Qt::AlignHCenter | Qt::AlignBottom, getColor(SAMPLE_SHALLOW));
				break;
			}
			if (data[new_from].depth < data[valley].depth)
				valley = new_from;
		}

		valley = act_peak.peak;

		// Search for first sample outside minimum range to the left.
		int new_to;
		for (new_to = act_peak.peak - 1; new_to >= act_peak.range_from + 3; --new_to) {
			if (data[new_to].sec + half_interval < act_sample.sec)
				break;
			if (data[new_to].depth < data[valley].depth)
				valley = new_to;
		}
		// Continue search until peaks reach the minimum prominence (height from valley).
		for ( ; new_to >= act_peak.range_from + 3; --new_to) {
			if (data[new_to].depth >= data[valley].depth + min_prominence) {
				int new_peak = std::max_element(data + act_peak.range_from, data + new_to, comp_depth) - data;
				if (data[new_peak].depth < min_depth)
					break;
				stack.push_back(Peak{ act_peak.range_from, new_to, new_peak });

				if (data[valley].depth >= min_depth)
					plot_depth_sample(data[valley], Qt::AlignHCenter | Qt::AlignBottom, getColor(SAMPLE_SHALLOW));
				break;
			}
			if (data[new_to].depth < data[valley].depth)
				valley = new_to;
		}
	}
}

void DiveProfileItem::plot_depth_sample(const struct plot_data &entry, QFlags<Qt::AlignmentFlag> flags, const QColor &color)
{
	auto item = std::make_unique<DiveTextItem>(dpr, 1.0, flags, this);
	item->set(get_depth_string(entry.depth, true), color);
	item->setPos(hAxis.posAtValue(entry.sec), vAxis.posAtValue(entry.depth));
	texts.push_back(std::move(item));
}

DiveHeartrateItem::DiveHeartrateItem(const plot_info &pInfo, const DiveCartesianAxis &hAxis,
				     const DiveCartesianAxis &vAxis, DataAccessor accessor, double dpr) :
	AbstractProfilePolygonItem(pInfo, hAxis, vAxis, accessor, dpr)
{
	QPen pen;
	pen.setBrush(QBrush(getColor(::HR_PLOT)));
	pen.setCosmetic(true);
	pen.setWidth(1);
	setPen(pen);
}

void DiveHeartrateItem::replot(const dive *, int fromIn, int toIn, bool)
{
	from = fromIn;
	to = toIn;

	int last = -300, last_printed_hr = 0;
	struct sec_hr {
		int sec;
		int hr;
	} hist[3] = {};
	std::vector<sec_hr> textItems;

	texts.clear();
	// Ignore empty values. a heart rate of 0 would be a bad sign.
	QPolygonF poly;
	int interval = vAxis.getMinLabelDistance(hAxis);
	for (int i = from; i < to; i++) {
		auto [sec_double, hr_double] = getPoint(i);
		int hr = lrint(hr_double);
		if (!hr)
			continue;
		int sec = lrint(sec_double);
		QPointF point(hAxis.posAtValue(sec_double), vAxis.posAtValue(hr_double));
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
		// if it's been less a full label interval and less than a 20 beats change OR
		// if it's been less than half a label interval OR if the change from the
		// last print is less than 10 beats
		// to test min / max requires three points, so we now look at the
		// previous one
		sec = hist[1].sec;
		hr = hist[1].hr;
		if ((hist[0].hr < hr && hr < hist[2].hr) ||
		    (hist[0].hr > hr && hr > hist[2].hr) ||
		    ((sec < last + interval) && (abs(hr - last_printed_hr) < 20)) ||
		    (sec < last + interval / 2) ||
		    (abs(hr - last_printed_hr) < 10))
			continue;
		last = sec;
		textItems.push_back({ sec, hr });
		last_printed_hr = hr;
	}
	setPolygon(poly);

	for (size_t i = 0; i < textItems.size(); ++i) {
		auto [sec, hr] = textItems[i];
		createTextItem(sec, hr, i == textItems.size() - 1);
	}
}

void DiveHeartrateItem::createTextItem(int sec, int hr, bool last)
{
	int flags = last ? Qt::AlignLeft | Qt::AlignBottom :
			   Qt::AlignRight | Qt::AlignBottom;
	auto text = std::make_unique<DiveTextItem>(dpr, 0.7, flags, this);
	text->set(QString("%1").arg(hr), getColor(HR_TEXT));
	text->setPos(QPointF(hAxis.posAtValue(sec), vAxis.posAtValue(hr)));
	texts.push_back(std::move(text));
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

DiveTemperatureItem::DiveTemperatureItem(const plot_info &pInfo, const DiveCartesianAxis &hAxis,
					 const DiveCartesianAxis &vAxis, DataAccessor accessor, double dpr) :
	AbstractProfilePolygonItem(pInfo, hAxis, vAxis, accessor, dpr)
{
	QPen pen;
	pen.setBrush(QBrush(getColor(::TEMP_PLOT)));
	pen.setCosmetic(true);
	pen.setWidth(2);
	setPen(pen);
}

void DiveTemperatureItem::replot(const dive *, int fromIn, int toIn, bool)
{
	from = fromIn;
	to = toIn;

	double last = -300.0, last_printed_temp = 0.0, last_valid_temp = 0.0, sec = 0.0;
	std::vector<std::pair<int, int>> textItems;

	texts.clear();
	// Ignore empty values. things do not look good with '0' as temperature in kelvin...
	QPolygonF poly;
	int interval = vAxis.getMinLabelDistance(hAxis);
	for (int i = from; i < to; i++) {
		auto [sec, mkelvin] = getPoint(i);
		if (mkelvin < 1.0)
			continue;
		QPointF point(hAxis.posAtValue(sec), vAxis.posAtValue(mkelvin));
		poly.append(point);
		last_valid_temp = sec;

		/* don't print a temperature
		 * if it's been less than a full label interval and less than a 2K change OR
		 * if it's been less than a half label interval OR if the change from the
		 * last print is less than .4K (and therefore less than 1F) */
		if (((sec < last + interval) && (fabs(mkelvin - last_printed_temp) < 2000.0)) ||
		    (sec < last + interval / 2) ||
		    (fabs(mkelvin - last_printed_temp) < 400.0))
			continue;
		last = sec;
		if (mkelvin > 200000.0)
			textItems.push_back({ static_cast<int>(sec), static_cast<int>(mkelvin) });
		last_printed_temp = mkelvin;
	}
	setPolygon(poly);

	/* print the end temperature, if it's different or if the
	 * last temperature print has been more than a quarter of the
	 * dive back */
	if (last_valid_temp > 200000.0 &&
	    ((fabs(last_valid_temp - last_printed_temp) > 500.0) || (last < 0.75 * sec))) {
		textItems.push_back({ static_cast<int>(sec), static_cast<int>(last_valid_temp) });
	}

	for (size_t i = 0; i < textItems.size(); ++i) {
		auto [sec, mkelvin] = textItems[i];
		createTextItem(sec, mkelvin, i == textItems.size() - 1);
	}
}

void DiveTemperatureItem::createTextItem(int sec, int mkelvin, bool last)
{
	temperature_t temp;
	temp.mkelvin = mkelvin;

	int flags = last ? Qt::AlignLeft | Qt::AlignBottom :
			   Qt::AlignRight | Qt::AlignBottom;
	auto text = std::make_unique<DiveTextItem>(dpr, 0.8, flags, this);
	text->set(get_temperature_string(temp, true), getColor(TEMP_TEXT));
	text->setPos(QPointF(hAxis.posAtValue(sec), vAxis.posAtValue(mkelvin)));
	texts.push_back(std::move(text));
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

static const double diveMeanDepthItemLabelScale = 0.8;

DiveMeanDepthItem::DiveMeanDepthItem(const plot_info &pInfo, const DiveCartesianAxis &hAxis,
				     const DiveCartesianAxis &vAxis, DataAccessor accessor, double dpr) :
	AbstractProfilePolygonItem(pInfo, hAxis, vAxis, accessor, dpr),
	labelWidth(DiveTextItem::getLabelSize(dpr, diveMeanDepthItemLabelScale, QStringLiteral("999.9ft")).first)
{
	QPen pen;
	pen.setBrush(QBrush(getColor(::HR_AXIS)));
	pen.setCosmetic(true);
	pen.setWidth(2);
	setPen(pen);
}

// Apparently, there can be samples without mean depth? If not, remove these functions.
std::pair<double,double> DiveMeanDepthItem::getMeanDepth(int i) const
{
	for ( ; i >= 0; --i) {
		const plot_data &entry = pInfo.entry[i];
		if (entry.running_sum > 0)
			return { static_cast<double>(entry.sec),
				 static_cast<double>(entry.running_sum) / entry.sec };
	}
	return { 0.0, 0.0 };
}

std::pair<double,double> DiveMeanDepthItem::getNextMeanDepth(int first) const
{
	int last = pInfo.nr;
	for (int i = first + 1; i < last;  ++i) {
		const plot_data &entry = pInfo.entry[i];
		if (entry.running_sum > 0)
			return { static_cast<double>(entry.sec),
				 static_cast<double>(entry.running_sum) / entry.sec };
	}
	return getMeanDepth(first);
}

void DiveMeanDepthItem::replot(const dive *, int fromIn, int toIn, bool)
{
	from = fromIn;
	to = toIn;

	double prevSec = 0.0, prevMeanDepth = 0.0;

	QPolygonF poly;
	for (int i = from; i < to; i++) {
		auto [sec, meanDepth] = getMeanDepth(i);
		// Ignore empty values
		if (meanDepth == 0)
			continue;
		if (i == from && i < to) {
			auto [sec2, meanDepth2] = getNextMeanDepth(i);
			if (meanDepth2 > 0.0)
				clipStart(sec, meanDepth, sec2, meanDepth2);
		}
		if (i == to - 1 && i > 0)
			clipStop(sec, meanDepth, prevSec, prevMeanDepth);


		QPointF point(hAxis.posAtValue(sec), vAxis.posAtValue(meanDepth));
		poly.append(point);

		prevSec = sec;
		prevMeanDepth = meanDepth;
	}
	setPolygon(poly);
	if (prevMeanDepth > 0.0)
		createTextItem(prevSec, prevMeanDepth);
}

void DiveMeanDepthItem::paint(QPainter *painter, const QStyleOptionGraphicsItem*, QWidget*)
{
	if (polygon().isEmpty())
		return;
	painter->save();
	painter->setPen(pen());
	painter->drawPolyline(polygon());
	painter->restore();
}

void DiveMeanDepthItem::createTextItem(double lastSec, double lastMeanDepth)
{
	texts.clear();
	auto text = std::make_unique<DiveTextItem>(dpr, diveMeanDepthItemLabelScale, Qt::AlignRight | Qt::AlignVCenter, this);
	text->set(get_depth_string(lrint(lastMeanDepth), true), getColor(TEMP_TEXT));
	text->setPos(QPointF(hAxis.posAtValue(lastSec) + dpr, vAxis.posAtValue(lastMeanDepth)));
	texts.push_back(std::move(text));
}

void DiveGasPressureItem::replot(const dive *d, int fromIn, int toIn, bool in_planner)
{
	from = fromIn;
	to = toIn;

	std::vector<int> plotted_cyl(pInfo.nr_cylinders, false);
	std::vector<double> last_plotted(pInfo.nr_cylinders, 0.0);
	std::vector<Segment> act_segments(pInfo.nr_cylinders);
	QPolygonF boundingPoly;
	segments.clear();

	for (int i = from; i < to; i++) {
		const struct plot_data *entry = pInfo.entry + i;

		for (int cyl = 0; cyl < pInfo.nr_cylinders; cyl++) {
			double mbar = static_cast<double>(get_plot_pressure(&pInfo, i, cyl));
			double time = static_cast<double>(entry->sec);

			if (mbar < 1.0)
				continue;

			if (i == from && i < to - 1) {
				double mbar2 = static_cast<double>(get_plot_pressure(&pInfo, i+1, cyl));
				double time2 = static_cast<double>(entry[1].sec);
				if (mbar2 < 1.0)
					continue;
				clipStart(time, mbar, time2, mbar2);
			}

			if (i == to - 1 && i > from) {
				double mbar2 = static_cast<double>(get_plot_pressure(&pInfo, i-1, cyl));
				double time2 = static_cast<double>(entry[-1].sec);
				if (mbar2 < 1.0)
					continue;
				clipStop(time, mbar, time2, mbar2);
			}

			QPointF point(hAxis.posAtValue(time), vAxis.posAtValue(mbar));
			boundingPoly.push_back(point);

			QColor color;
			if (!in_planner) {
				if (entry->sac)
					color = getSacColor(entry->sac, d->sac);
				else
					color = MED_GRAY_HIGH_TRANS;
			} else {
				if (mbar < 0.0)
					color = MAGENTA;
				else
					color = getPressureColor(entry->density);
			}

			if (!act_segments[cyl].polygon.empty()) {
				/* Have we used this cylinder in the last two minutes? Continue */
				if (time - act_segments[cyl].last.time <= 2*60) {
					act_segments[cyl].polygon.push_back({ point, color });
					act_segments[cyl].last.time = time;
					act_segments[cyl].last.pressure = mbar;
					continue;
				}

				/* Finish the previous one, start a new one */
				act_segments[cyl].cyl = cyl;
				segments.push_back(std::move(act_segments[cyl]));
				act_segments[cyl] = Segment();
			}

			plotted_cyl[cyl] = true;
			act_segments[cyl].polygon.push_back({ point, color });
			act_segments[cyl].last.time = time;
			act_segments[cyl].last.pressure = mbar;
			if (act_segments[cyl].first.pressure == 0.0) {
				act_segments[cyl].first.time = time;
				act_segments[cyl].first.pressure = mbar;
			}
		}
	}

	for (int cyl = 0; cyl < pInfo.nr_cylinders; cyl++) {
		if (act_segments[cyl].polygon.empty())
			continue;
		act_segments[cyl].cyl = cyl;
		segments.push_back(std::move(act_segments[cyl]));
	}

	setPolygon(boundingPoly);
	texts.clear();

	// These are offset values used to print the gas labels and pressures on a
	// dive profile at appropriate Y-coordinates. We alternate aligning the
	// label and the gas pressure above and under the pressure line.
	// The values are historical, and we could try to pick the over/under
	// depending on whether this pressure is higher or lower than the average.
	// Right now it's just strictly alternating when you have multiple gas
	// pressures.

	QFlags<Qt::AlignmentFlag> alignVar = Qt::AlignTop;
	std::vector<QFlags<Qt::AlignmentFlag>> align(pInfo.nr_cylinders);

	double labelHeight = DiveTextItem::fontHeight(dpr, 1.0);

	for (const Segment &segment: segments) {
		// Magic Y offset depending on whether we're aliging
		// the top of the text or the bottom of the text to
		// the pressure line.
		double value_y_offset = -0.5 * dpr;
		double label_y_offset = alignVar & Qt::AlignTop ? labelHeight : -labelHeight;
		gasmix gas = get_cylinder(d, segment.cyl)->gasmix;
		plotPressureValue(segment.first.pressure, segment.first.time, alignVar, value_y_offset);
		plotGasValue(segment.first.pressure, segment.first.time, gas, alignVar, label_y_offset);

		// For each cylinder, on right hand side of the curve, write cylinder pressure
		plotPressureValue(segment.last.pressure, segment.last.time, alignVar | Qt::AlignLeft, value_y_offset);

		/* Alternate alignment as we see cylinder use.. */
		alignVar ^= Qt::AlignTop | Qt::AlignBottom;
	}
}

void DiveGasPressureItem::plotPressureValue(double mbar, double sec, QFlags<Qt::AlignmentFlag> align, double pressure_offset)
{
	const char *unit;
	int pressure = get_pressure_units(lrint(mbar), &unit);
	auto text = std::make_unique<DiveTextItem>(dpr, 1.0, align, this);
	text->set(QString("%1%2").arg(pressure).arg(unit), getColor(PRESSURE_TEXT));
	text->setPos(hAxis.posAtValue(sec), vAxis.posAtValue(mbar) + pressure_offset);
	texts.push_back(std::move(text));
}

void DiveGasPressureItem::plotGasValue(double mbar, double sec, struct gasmix gasmix, QFlags<Qt::AlignmentFlag> align, double gasname_offset)
{
	QString gas = get_gas_string(gasmix);
	auto text = std::make_unique<DiveTextItem>(dpr, 1.0, align, this);
	text->set(gas, getColor(PRESSURE_TEXT));
	text->setPos(hAxis.posAtValue(sec), vAxis.posAtValue(mbar) + gasname_offset);
	texts.push_back(std::move(text));
}

void DiveGasPressureItem::paint(QPainter *painter, const QStyleOptionGraphicsItem*, QWidget*)
{
	if (polygon().isEmpty())
		return;
	QPen pen;
	pen.setCosmetic(true);
	pen.setWidth(2);
	painter->save();
	for (const Segment &segment: segments) {
		for (size_t i = 1; i < segment.polygon.size(); i++) {
			pen.setBrush(segment.polygon[i].col);
			painter->setPen(pen);
			painter->drawLine(segment.polygon[i - 1].pos, segment.polygon[i].pos);
		}
	}
	painter->restore();
}

DiveCalculatedCeiling::DiveCalculatedCeiling(const plot_info &pInfo, const DiveCartesianAxis &hAxis,
					     const DiveCartesianAxis &vAxis, DataAccessor accessor, double dpr) :
	AbstractProfilePolygonItem(pInfo, hAxis, vAxis, accessor, dpr)
{
}

void DiveCalculatedCeiling::replot(const dive *d, int from, int to, bool in_planner)
{
	makePolygon(from, to);

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

DiveCalculatedTissue::DiveCalculatedTissue(const plot_info &pInfo, const DiveCartesianAxis &hAxis,
					   const DiveCartesianAxis &vAxis, DataAccessor accessor, double dpr) :
	DiveCalculatedCeiling(pInfo, hAxis, vAxis, accessor, dpr)
{
}

DiveReportedCeiling::DiveReportedCeiling(const plot_info &pInfo, const DiveCartesianAxis &hAxis,
					 const DiveCartesianAxis &vAxis, DataAccessor accessor, double dpr) :
	AbstractProfilePolygonItem(pInfo, hAxis, vAxis, accessor, dpr)
{
}

std::pair<double,double> DiveReportedCeiling::getTimeValue(int i) const
{
	const plot_data &entry = pInfo.entry[i];
	int value = entry.in_deco && entry.stopdepth ? std::min(entry.stopdepth, entry.depth) : 0;
	return { static_cast<double>(entry.sec), static_cast<double>(value) };
}

std::pair<double, double> DiveReportedCeiling::getPoint(int i) const
{
	auto [x,y] = getTimeValue(i);
	if (i == from && i < to) {
		auto [next_x, next_y] = getTimeValue(i + 1);
		clipStart(x, y, next_x, next_y);
	}
	if (i == to - 1 && i > 0) {
		auto [prev_x, prev_y] = getTimeValue(i - 1);
		clipStop(x, y, prev_x, prev_y);
	}
	return { x, y };
}

void DiveReportedCeiling::replot(const dive *, int fromIn, int toIn, bool)
{
	from = fromIn;
	to = toIn;

	QPolygonF p;
	for (int i = from; i < to; i++) {
		auto [sec, value] = getPoint(i);
		if (i == from)
			p.append(QPointF(hAxis.posAtValue(sec), vAxis.posAtValue(0.0)));
		p.append(QPointF(hAxis.posAtValue(sec), vAxis.posAtValue(value)));
		if (i == to - 1)
			p.append(QPointF(hAxis.posAtValue(sec), vAxis.posAtValue(0)));
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

void PartialPressureGasItem::replot(const dive *, int fromIn, int toIn, bool)
{
	from = fromIn;
	to = toIn;

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
	for (int i = from; i < to; i++) {
		auto [time, value] = getPoint(i);
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
}

void PartialPressureGasItem::paint(QPainter *painter, const QStyleOptionGraphicsItem*, QWidget*)
{
	const qreal pWidth = 0.0;
	painter->save();
	painter->setPen(QPen(normalColor, pWidth));
	painter->drawPolyline(polygon());

	QPolygonF poly;
	painter->setPen(QPen(alertColor, pWidth));
	for (const QPolygonF &poly: alertPolygons)
		painter->drawPolyline(poly);
	painter->restore();
}

void PartialPressureGasItem::setThresholdSettingsKey(const double *prefPointerMin, const double *prefPointerMax)
{
	thresholdPtrMin = prefPointerMin;
	thresholdPtrMax = prefPointerMax;
}

PartialPressureGasItem::PartialPressureGasItem(const plot_info &pInfo, const DiveCartesianAxis &hAxis,
					       const DiveCartesianAxis &vAxis, DataAccessor accessor, double dpr) :
	AbstractProfilePolygonItem(pInfo, hAxis, vAxis, accessor, dpr),
	thresholdPtrMin(NULL),
	thresholdPtrMax(NULL)
{
}

void PartialPressureGasItem::setColors(const QColor &normal, const QColor &alert)
{
	normalColor = normal;
	alertColor = alert;
}
