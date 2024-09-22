// SPDX-License-Identifier: GPL-2.0
#include "profile-widget/diveeventitem.h"
#include "profile-widget/divecartesianaxis.h"
#include "profile-widget/divepixmapcache.h"
#include "profile-widget/animationfunctions.h"
#include "core/dive.h"
#include "core/event.h"
#include "core/eventtype.h"
#include "core/format.h"
#include "core/profile.h"
#include "core/gettextfromc.h"
#include "core/sample.h"
#include "core/subsurface-string.h"

#define DEPTH_NOT_FOUND (-2342)

static int depthAtTime(const plot_info &pi, duration_t time);

DiveEventItem::DiveEventItem(const struct dive *d, const struct divecomputer *dc, int idx, const struct event &ev, const struct gasmix lastgasmix, divemode_t lastdivemode,
			     const plot_info &pi, DiveCartesianAxis *hAxis, DiveCartesianAxis *vAxis,
			     int speed, const DivePixmaps &pixmaps, QGraphicsItem *parent) : DivePixmapItem(parent),
	vAxis(vAxis),
	hAxis(hAxis),
	idx(idx),
	ev(ev),
	dive(d),
	depth(depthAtTime(pi, ev.time))
{
	setFlag(ItemIgnoresTransformations);

	setupPixmap(lastgasmix, lastdivemode, *dc, pixmaps);
	setupToolTipString(lastgasmix, lastdivemode, *dc);
	recalculatePos();
}

DiveEventItem::~DiveEventItem()
{
}

void DiveEventItem::setupPixmap(const struct gasmix lastgasmix, divemode_t lastdivemode, const struct divecomputer &dc, const DivePixmaps &pixmaps)
{
	event_severity severity = ev.get_severity();
	if (ev.name.empty()) {
		setPixmap(pixmaps.warning);
	} else if (same_string_caseinsensitive(ev.name.c_str(), "modechange")) {
		if (ev.value == 0)
			setPixmap(pixmaps.bailout);
		else
			setPixmap(pixmaps.onCCRLoop);
	} else if (ev.type == SAMPLE_EVENT_BOOKMARK) {
		setPixmap(pixmaps.bookmark);
		setOffset(QPointF(0.0, -pixmap().height()));
	} else if (ev.is_gaschange()) {
		auto [mix, divemode] = dive->get_gasmix_from_event(ev, dc);
		struct icd_data icd_data;
		bool icd = isobaric_counterdiffusion(lastgasmix, mix, &icd_data);
		if (divemode != lastdivemode) {
			if (divemode == CCR)
				setPixmap(pixmaps.onCCRLoop);
			else
				setPixmap(pixmaps.bailout);
		} else if (mix.he.permille) {
			if (icd)
				setPixmap(pixmaps.gaschangeTrimixICD);
			else
				setPixmap(pixmaps.gaschangeTrimix);
		} else if (gasmix_is_air(mix)) {
			if (icd)
				setPixmap(pixmaps.gaschangeAirICD);
			else
				setPixmap(pixmaps.gaschangeAir);
		} else if (mix.o2.permille == 1000) {
			if (icd)
				setPixmap(pixmaps.gaschangeOxygenICD);
			else
				setPixmap(pixmaps.gaschangeOxygen);
		} else {
			if (icd)
				setPixmap(pixmaps.gaschangeEANICD);
			else
				setPixmap(pixmaps.gaschangeEAN);
		}
	} else if ((((ev.flags & SAMPLE_FLAGS_SEVERITY_MASK) >> SAMPLE_FLAGS_SEVERITY_SHIFT) == 1) ||
		    // those are useless internals of the dive computer
		   (same_string_caseinsensitive(ev.name.c_str(), "SP change") && ev.time.seconds == 0)) {
		// at t=0 we might have an "SP change" to indicate dive type
		// we want to get the right data into the tooltip but don't want the visual clutter
		// so set an "almost invisible" pixmap (a narrow but somewhat tall, basically transparent pixmap)
		// that allows tooltips to work when we don't want to show a specific
		// pixmap for an event, but want to show the event value in the tooltip
		setPixmap(pixmaps.transparent);
	} else if (severity == EVENT_SEVERITY_INFO) {
		setPixmap(pixmaps.info);
	} else if (severity == EVENT_SEVERITY_WARN) {
		setPixmap(pixmaps.warning);
	} else if (severity == EVENT_SEVERITY_ALARM) {
		setPixmap(pixmaps.violation);
	} else if (same_string_caseinsensitive(ev.name.c_str(), "violation") || // generic libdivecomputer
		   same_string_caseinsensitive(ev.name.c_str(), "Safety stop violation")  || // the rest are from the Uemis downloader
		   same_string_caseinsensitive(ev.name.c_str(), "pO₂ ascend alarm")  ||
		   same_string_caseinsensitive(ev.name.c_str(), "RGT alert")  ||
		   same_string_caseinsensitive(ev.name.c_str(), "Dive time alert")  ||
		   same_string_caseinsensitive(ev.name.c_str(), "Low battery alert")  ||
		   same_string_caseinsensitive(ev.name.c_str(), "Speed alarm")) {
		setPixmap(pixmaps.violation);
	} else if (same_string_caseinsensitive(ev.name.c_str(), "non stop time") || // generic libdivecomputer
		   same_string_caseinsensitive(ev.name.c_str(), "safety stop") ||
		   same_string_caseinsensitive(ev.name.c_str(), "safety stop (voluntary)") ||
		   same_string_caseinsensitive(ev.name.c_str(), "Tank change suggested") || // Uemis downloader
		   same_string_caseinsensitive(ev.name.c_str(), "Marker")) {
		setPixmap(pixmaps.info);
	} else {
		// we should do some guessing based on the type / name of the event;
		// for now they all get the warning icon
		setPixmap(pixmaps.warning);
	}
}

void DiveEventItem::setupToolTipString(const struct gasmix lastgasmix, divemode_t lastdivemode, const struct divecomputer &dc)
{
	// we display the event on screen - so translate
	QString name = gettextFromC::tr(ev.name.c_str());
	int value = ev.value;
	int type = ev.type;

	if (ev.is_gaschange()) {
		struct icd_data icd_data;
		auto [mix, divemode] = dive->get_gasmix_from_event(ev, dc);
		name += ": ";
		name += QString::fromStdString(mix.name());

		/* Do we have an explicit cylinder index?  Show it. */
		if (ev.gas.index >= 0)
			name += tr(" (cyl. %1)").arg(ev.gas.index + 1);
		bool icd = isobaric_counterdiffusion(lastgasmix, mix, &icd_data);
		if (icd_data.dHe < 0) {
			name += qasprintf_loc("\n%s %s:%+.3g%% %s:%+.3g%%%s%+.3g%%",
					      qPrintable(tr("ICD")),
					      qPrintable(tr("ΔHe")), icd_data.dHe / 10.0,
					      qPrintable(tr("ΔN₂")), icd_data.dN2 / 10.0,
					      icd ? ">" : "<", lrint(-icd_data.dHe / 5.0) / 10.0);
		}
		if (divemode != lastdivemode)
			name += QString("\nmodechange: %1").arg(gettextFromC::tr(divemode_text_ui[divemode != OC]));

	} else if (ev.name == "modechange") {
		name += QString(": %1").arg(gettextFromC::tr(divemode_text_ui[ev.value]));
	} else if (value) {
		if (type == SAMPLE_EVENT_PO2 && ev.name == "SP change") {
			name += QString(": %1bar").arg((double)value / 1000, 0, 'f', 1);
		} else if (type == SAMPLE_EVENT_CEILING && ev.name == "planned waypoint above ceiling") {
			const char *depth_unit;
			depth_t depth { .mm = value * 1000 };
			double depth_value = get_depth_units(depth, NULL, &depth_unit);
			name += QString(": %1%2").arg((int) round(depth_value)).arg(depth_unit);
		} else {
			name += QString(": %1").arg(value);
		}
	} else if (type == SAMPLE_EVENT_PO2 && ev.name == "SP change") {
		// this is a bad idea - we are abusing an existing event type that is supposed to
		// warn of high or low pO₂ and are turning it into a setpoint change event
		name += ":\n" + tr("Manual switch to OC");
	} else {
		name += ev.flags & SAMPLE_FLAGS_BEGIN ? tr(" begin", "Starts with space!") :
								    ev.flags & SAMPLE_FLAGS_END ? tr(" end", "Starts with space!") : "";
	}
	setToolTip(QString("<img height=\"16\" src=\":status-warning-icon\">&nbsp;  ") + name);
}

void DiveEventItem::eventVisibilityChanged(const QString&, bool)
{
	//WARN: lookslike we should implement this.
}

static int depthAtTime(const plot_info &pi, duration_t time)
{
	// Do a binary search for the timestamp
	auto it = std::lower_bound(pi.entry.begin(), pi.entry.end(), time,
				   [](const plot_data &d1, duration_t t) { return d1.sec < t.seconds; });
	if (it == pi.entry.end() || it->sec != time.seconds) {
		qWarning("can't find a spot in the dataModel");
		return DEPTH_NOT_FOUND;
	}
	return it->depth.mm;
}

bool DiveEventItem::isInteresting(const struct dive *d, const struct divecomputer *dc,
				  const struct event &ev, const plot_info &pi,
				  int firstSecond, int lastSecond)
{
	/*
	 * Ignore items outside of plot range
	 */
	if (ev.time.seconds < firstSecond || ev.time.seconds >= lastSecond)
		return false;

	/*
	 * Some gas change events are special. Some dive computers just tell us the initial gas this way.
	 * Don't bother showing those
	 */
	if (ev.name == "gaschange" &&
	    (ev.time.seconds == 0 ||
	     (!dc->samples.empty() && ev.time.seconds == dc->samples[0].time.seconds) ||
	     depthAtTime(pi, ev.time) < SURFACE_THRESHOLD))
		return false;

	/*
	 * Some divecomputers give "surface" events that just aren't interesting.
	 * Like at the beginning or very end of a dive. Well, duh.
	 */
	if (ev.name == "surface") {
		int time = ev.time.seconds;
		if (time <= 30 || time + 30 >= (int)dc->duration.seconds)
			return false;
	}
	return true;
}

void DiveEventItem::recalculatePos()
{
	if (depth == DEPTH_NOT_FOUND) {
		hide();
		return;
	}
	setVisible(!ev.hidden && !is_event_type_hidden(&ev));
	double x = hAxis->posAtValue(ev.time.seconds);
	double y = vAxis->posAtValue(depth);
	setPos(x, y);
}
