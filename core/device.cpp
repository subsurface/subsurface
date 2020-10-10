// SPDX-License-Identifier: GPL-2.0
#include "ssrf.h"
#include "dive.h"
#include "subsurface-string.h"
#include "qthelper.h" // for copy_qstring
#include "device.h"
#include "errorhelper.h" // for verbose flag
#include "selection.h"
#include "core/settings/qPrefDiveComputer.h"

/*
 * Good fake dive profiles are hard.
 *
 * "depthtime" is the integral of the dive depth over
 * time ("area" of the dive profile). We want that
 * area to match the average depth (avg_d*max_t).
 *
 * To do that, we generate a 6-point profile:
 *
 *  (0, 0)
 *  (t1, max_d)
 *  (t2, max_d)
 *  (t3, d)
 *  (t4, d)
 *  (max_t, 0)
 *
 * with the same ascent/descent rates between the
 * different depths.
 *
 * NOTE: avg_d, max_d and max_t are given constants.
 * The rest we can/should play around with to get a
 * good-looking profile.
 *
 * That six-point profile gives a total area of:
 *
 *   (max_d*max_t) - (max_d*t1) - (max_d-d)*(t4-t3)
 *
 * And the "same ascent/descent rates" requirement
 * gives us (time per depth must be same):
 *
 *   t1 / max_d = (t3-t2) / (max_d-d)
 *   t1 / max_d = (max_t-t4) / d
 *
 * We also obviously require:
 *
 *   0 <= t1 <= t2 <= t3 <= t4 <= max_t
 *
 * Let us call 'd_frac = d / max_d', and we get:
 *
 * Total area must match average depth-time:
 *
 *   (max_d*max_t) - (max_d*t1) - (max_d-d)*(t4-t3) = avg_d*max_t
 *      max_d*(max_t-t1-(1-d_frac)*(t4-t3)) = avg_d*max_t
 *             max_t-t1-(1-d_frac)*(t4-t3) = avg_d*max_t/max_d
 *                   t1+(1-d_frac)*(t4-t3) = max_t*(1-avg_d/max_d)
 *
 * and descent slope must match ascent slopes:
 *
 *   t1 / max_d = (t3-t2) / (max_d*(1-d_frac))
 *           t1 = (t3-t2)/(1-d_frac)
 *
 * and
 *
 *   t1 / max_d = (max_t-t4) / (max_d*d_frac)
 *           t1 = (max_t-t4)/d_frac
 *
 * In general, we have more free variables than we have constraints,
 * but we can aim for certain basics, like a good ascent slope.
 */
static int fill_samples(struct sample *s, int max_d, int avg_d, int max_t, double slope, double d_frac)
{
	double t_frac = max_t * (1 - avg_d / (double)max_d);
	int t1 = lrint(max_d / slope);
	int t4 = lrint(max_t - t1 * d_frac);
	int t3 = lrint(t4 - (t_frac - t1) / (1 - d_frac));
	int t2 = lrint(t3 - t1 * (1 - d_frac));

	if (t1 < 0 || t1 > t2 || t2 > t3 || t3 > t4 || t4 > max_t)
		return 0;

	s[1].time.seconds = t1;
	s[1].depth.mm = max_d;
	s[2].time.seconds = t2;
	s[2].depth.mm = max_d;
	s[3].time.seconds = t3;
	s[3].depth.mm = lrint(max_d * d_frac);
	s[4].time.seconds = t4;
	s[4].depth.mm = lrint(max_d * d_frac);

	return 1;
}

/* we have no average depth; instead of making up a random average depth
 * we should assume either a PADI rectangular profile (for short and/or
 * shallow dives) or more reasonably a six point profile with a 3 minute
 * safety stop at 5m */
static void fill_samples_no_avg(struct sample *s, int max_d, int max_t, double slope)
{
	// shallow or short dives are just trapecoids based on the given slope
	if (max_d < 10000 || max_t < 600) {
		s[1].time.seconds = lrint(max_d / slope);
		s[1].depth.mm = max_d;
		s[2].time.seconds = max_t - lrint(max_d / slope);
		s[2].depth.mm = max_d;
	} else {
		s[1].time.seconds = lrint(max_d / slope);
		s[1].depth.mm = max_d;
		s[2].time.seconds = max_t - lrint(max_d / slope) - 180;
		s[2].depth.mm = max_d;
		s[3].time.seconds = max_t - lrint(5000 / slope) - 180;
		s[3].depth.mm = 5000;
		s[4].time.seconds = max_t - lrint(5000 / slope);
		s[4].depth.mm = 5000;
	}
}

extern "C" void fake_dc(struct divecomputer *dc)
{
	alloc_samples(dc, 6);
	struct sample *fake = dc->sample;
	int i;

	dc->samples = 6;

	/* The dive has no samples, so create a few fake ones */
	int max_t = dc->duration.seconds;
	int max_d = dc->maxdepth.mm;
	int avg_d = dc->meandepth.mm;

	memset(fake, 0, 6 * sizeof(struct sample));
	fake[5].time.seconds = max_t;
	for (i = 0; i < 6; i++) {
		fake[i].bearing.degrees = -1;
		fake[i].ndl.seconds = -1;
	}
	if (!max_t || !max_d) {
		dc->samples = 0;
		return;
	}

	/* Set last manually entered time to the total dive length */
	dc->last_manual_time = dc->duration;

	/*
	 * We want to fake the profile so that the average
	 * depth ends up correct. However, in the absence of
	 * a reasonable average, let's just make something
	 * up. Note that 'avg_d == max_d' is _not_ a reasonable
	 * average.
	 * We explicitly treat avg_d == 0 differently */
	if (avg_d == 0) {
		/* we try for a sane slope, but bow to the insanity of
		 * the user supplied data */
		fill_samples_no_avg(fake, max_d, max_t, MAX(2.0 * max_d / max_t, (double)prefs.ascratelast6m));
		if (fake[3].time.seconds == 0) { // just a 4 point profile
			dc->samples = 4;
			fake[3].time.seconds = max_t;
		}
		return;
	}
	if (avg_d < max_d / 10 || avg_d >= max_d) {
		avg_d = (max_d + 10000) / 3;
		if (avg_d > max_d)
			avg_d = max_d * 2 / 3;
	}
	if (!avg_d)
		avg_d = 1;

	/*
	 * Ok, first we try a basic profile with a specific ascent
	 * rate (5 meters per minute) and d_frac (1/3).
	 */
	if (fill_samples(fake, max_d, avg_d, max_t, (double)prefs.ascratelast6m, 0.33))
		return;

	/*
	 * Ok, assume that didn't work because we cannot make the
	 * average come out right because it was a quick deep dive
	 * followed by a much shallower region
	 */
	if (fill_samples(fake, max_d, avg_d, max_t, 10000.0 / 60, 0.10))
		return;

	/*
	 * Uhhuh. That didn't work. We'd need to find a good combination that
	 * satisfies our constraints. Currently, we don't, we just give insane
	 * slopes.
	 */
	if (fill_samples(fake, max_d, avg_d, max_t, 10000.0, 0.01))
		return;

	/* Even that didn't work? Give up, there's something wrong */
}

struct device_table device_table;

bool device::operator==(const device &a) const
{
	return model == a.model &&
	       deviceId == a.deviceId &&
	       firmware == a.firmware &&
	       serialNumber == a.serialNumber &&
	       nickName == a.nickName;
}

bool device::operator!=(const device &a) const
{
	return !(*this == a);
}

bool device::operator<(const device &a) const
{
	return std::tie(deviceId, model) < std::tie(a.deviceId, a.model);
}

static const device *getDCExact(const QVector<device> &dcs, const divecomputer *dc)
{
	auto it = std::lower_bound(dcs.begin(), dcs.end(), device{dc->model, dc->deviceid, {}, {}, {}});
	return it != dcs.end() && it->model == dc->model && it->deviceId == dc->deviceid ? &*it : NULL;
}

static const device *getDC(const QVector<device> &dcs, const divecomputer *dc)
{
	auto it = std::lower_bound(dcs.begin(), dcs.end(), device{dc->model, 0, {}, {}, {}});
	return it != dcs.end() && it->model == dc->model ? &*it : NULL;
}

/*
 * When setting the device ID, we also fill in the
 * serial number and firmware version data
 */
extern "C" void set_dc_deviceid(struct divecomputer *dc, unsigned int deviceid)
{
	if (!deviceid)
		return;

	dc->deviceid = deviceid;

	// Serial and firmware can only be deduced if we know the model
	if (!dc->model)
		return;

	const device *node = getDCExact(device_table.devices, dc);
	if (!node)
		return;

	if (!node->serialNumber.isEmpty() && empty_string(dc->serial))
		dc->serial = copy_qstring(node->serialNumber);
	if (!node->firmware.isEmpty() && empty_string(dc->fw_version))
		dc->fw_version = copy_qstring(node->firmware);
}

void device::showchanges(const QString &n, const QString &s, const QString &f) const
{
	if (nickName != n && !n.isEmpty())
		qDebug("new nickname %s for DC model %s deviceId 0x%x", qPrintable(n), qPrintable(model), deviceId);
	if (serialNumber != s && !s.isEmpty())
		qDebug("new serial number %s for DC model %s deviceId 0x%x", qPrintable(s), qPrintable(model), deviceId);
	if (firmware != f && !f.isEmpty())
		qDebug("new firmware version %s for DC model %s deviceId 0x%x", qPrintable(f), qPrintable(model), deviceId);
}

static void addDC(QVector<device> &dcs, const QString &m, uint32_t d, const QString &n, const QString &s, const QString &f)
{
	if (m.isEmpty() || d == 0)
		return;
	auto it = std::lower_bound(dcs.begin(), dcs.end(), device{m, d, {}, {}, {}});
	if (it != dcs.end() && it->model == m && it->deviceId == d) {
		// debugging: show changes
		if (verbose)
			it->showchanges(n, s, f);
		// Update any non-existent fields from the old entry
		if (!n.isEmpty())
			it->nickName = n;
		if (!s.isEmpty())
			it->serialNumber = s;
		if (!f.isEmpty())
			it->firmware = f;
	} else {
		dcs.insert(it, device{m, d, s, f, n});
	}
}

extern "C" void create_device_node(const char *model, uint32_t deviceid, const char *serial, const char *firmware, const char *nickname)
{
	addDC(device_table.devices, model, deviceid, nickname, serial, firmware);
}

extern "C" void clear_device_nodes()
{
	device_table.devices.clear();
}

extern "C" void call_for_each_dc (void *f, void (*callback)(void *, const char *, uint32_t, const char *, const char *, const char *),
				  bool select_only)
{
	for (const device &node : device_table.devices) {
		bool found = false;
		if (select_only) {
			for (dive *d: getDiveSelection()) {
				struct divecomputer *dc;
				for_each_dc (d, dc) {
					if (dc->deviceid == node.deviceId) {
						found = true;
						break;
					}
				}
				if (found)
					break;
			}
		} else {
			found = true;
		}
		if (found)
			callback(f, qPrintable(node.model), node.deviceId, qPrintable(node.nickName),
						 qPrintable(node.serialNumber), qPrintable(node.firmware));
	}
}

extern "C" int is_default_dive_computer(const char *vendor, const char *product)
{
	return qPrefDiveComputer::vendor() == vendor && qPrefDiveComputer::product() == product;
}

extern "C" int is_default_dive_computer_device(const char *name)
{
	return qPrefDiveComputer::device() == name;
}

extern "C" void set_dc_nickname(struct dive *dive)
{
	if (!dive)
		return;

	struct divecomputer *dc;

	for_each_dc (dive, dc) {
		if (!empty_string(dc->model) && dc->deviceid &&
		    !getDCExact(device_table.devices, dc)) {
			// we don't have this one, yet
			const device *existNode = getDC(device_table.devices, dc);
			if (existNode) {
				// we already have this model but a different deviceid
				QString simpleNick(dc->model);
				if (dc->deviceid == 0)
					simpleNick.append(" (unknown deviceid)");
				else
					simpleNick.append(" (").append(QString::number(dc->deviceid, 16)).append(")");
				addDC(device_table.devices, dc->model, dc->deviceid, simpleNick, {}, {});
			} else {
				addDC(device_table.devices, dc->model, dc->deviceid, {}, {}, {});
			}
		}
	}
}

QString get_dc_nickname(const struct divecomputer *dc)
{
	const device *existNode = getDCExact(device_table.devices, dc);

	if (existNode && !existNode->nickName.isEmpty())
		return existNode->nickName;
	else
		return dc->model;
}
