// SPDX-License-Identifier: GPL-2.0
#include "ssrf.h"
#include "dive.h"
#include "subsurface-string.h"
#include "device.h"
#include "errorhelper.h" // for verbose flag
#include "selection.h"
#include "core/settings/qPrefDiveComputer.h"
#include <QString> // for QString::number

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

static bool same_device(const device &dev1, const device &dev2)
{
	return dev1.deviceId == dev2.deviceId && strcoll(dev1.model.c_str(), dev2.model.c_str()) == 0;
}

bool device::operator<(const device &a) const
{
	if (deviceId != a.deviceId)
		return deviceId < a.deviceId;

	// Use strcoll to compare model-strings, since these might be unicode
	// and therefore locale dependent? Let's hope that not, but who knows?
	return strcoll(model.c_str(), a.model.c_str()) < 0;
}

extern "C" const struct device *get_device_for_dc(const struct device_table *table, const struct divecomputer *dc)
{
	const std::vector<device> &dcs = table->devices;
	device dev { dc->model, dc->deviceid, {}, {}, {} };
	auto it = std::lower_bound(dcs.begin(), dcs.end(), dev);
	return it != dcs.end() && same_device(*it, dev) ? &*it : NULL;
}

extern "C" bool device_exists(const struct device_table *device_table, const struct device *dev)
{
	auto it = std::lower_bound(device_table->devices.begin(), device_table->devices.end(), *dev);
	return it != device_table->devices.end() && same_device(*it, *dev);
}

/*
 * When setting the device ID, we also fill in the
 * serial number and firmware version data
 */
extern "C" void set_dc_deviceid(struct divecomputer *dc, unsigned int deviceid, const struct device_table *device_table)
{
	if (!deviceid)
		return;

	dc->deviceid = deviceid;

	// Serial and firmware can only be deduced if we know the model
	if (!dc->model)
		return;

	const device *node = get_device_for_dc(device_table, dc);
	if (!node)
		return;

	if (!node->serialNumber.empty() && empty_string(dc->serial)) {
		free((void *)dc->serial);
		dc->serial = strdup(node->serialNumber.c_str());
	}
	if (!node->firmware.empty() && empty_string(dc->fw_version)) {
		free((void *)dc->fw_version);
		dc->fw_version = strdup(node->firmware.c_str());
	}
}

void device::showchanges(const std::string &n, const std::string &s, const std::string &f) const
{
	if (nickName != n && !n.empty())
		qDebug("new nickname %s for DC model %s deviceId 0x%x", n.c_str(), model.c_str(), deviceId);
	if (serialNumber != s && !s.empty())
		qDebug("new serial number %s for DC model %s deviceId 0x%x", s.c_str(), model.c_str(), deviceId);
	if (firmware != f && !f.empty())
		qDebug("new firmware version %s for DC model %s deviceId 0x%x", f.c_str(), model.c_str(), deviceId);
}

static void addDC(std::vector<device> &dcs, const std::string &m, uint32_t d, const std::string &n, const std::string &s, const std::string &f)
{
	if (m.empty() || d == 0)
		return;
	device dev { m, d, {}, {}, {} };
	auto it = std::lower_bound(dcs.begin(), dcs.end(), dev);
	if (it != dcs.end() && same_device(*it, dev)) {
		// debugging: show changes
		if (verbose)
			it->showchanges(n, s, f);
		// Update any non-existent fields from the old entry
		if (!n.empty())
			it->nickName = n;
		if (!s.empty())
			it->serialNumber = s;
		if (!f.empty())
			it->firmware = f;
	} else {
		dcs.insert(it, device{m, d, s, f, n});
	}
}

extern "C" void create_device_node(struct device_table *device_table, const char *model, uint32_t deviceid, const char *serial, const char *firmware, const char *nickname)
{
	addDC(device_table->devices, model ?: "", deviceid, nickname ?: "", serial ?: "", firmware ?: "");
}

/* Does not check for duplicates! */
extern "C" int add_to_device_table(struct device_table *device_table, const struct device *dev)
{
	auto it = std::lower_bound(device_table->devices.begin(), device_table->devices.end(), *dev);
	int idx = it - device_table->devices.begin();
	device_table->devices.insert(it, *dev);
	return idx;
}

extern "C" int remove_device(struct device_table *device_table, const struct device *dev)
{
	auto it = std::lower_bound(device_table->devices.begin(), device_table->devices.end(), *dev);
	if (it != device_table->devices.end() && same_device(*it, *dev)) {
		int idx = it - device_table->devices.begin();
		device_table->devices.erase(it);
		return idx;
	} else {
		return -1;
	}
}

extern "C" void clear_device_table(struct device_table *device_table)
{
	device_table->devices.clear();
}

/* Returns whether the given device is used by a selected dive. */
extern "C" bool device_used_by_selected_dive(const struct device *dev)
{
	for (dive *d: getDiveSelection()) {
		struct divecomputer *dc;
		for_each_dc (d, dc) {
			if (dc->deviceid == dev->deviceId)
				return true;
		}
	}
	return false;
}

extern "C" int is_default_dive_computer_device(const char *name)
{
	return qPrefDiveComputer::device() == name;
}

extern "C" void set_dc_nickname(struct dive *dive, struct device_table *device_table)
{
	if (!dive)
		return;

	struct divecomputer *dc;

	for_each_dc (dive, dc) {
		if (!empty_string(dc->model) && dc->deviceid &&
		    !get_device_for_dc(device_table, dc)) {
			// we don't have this one, yet
			if (std::any_of(device_table->devices.begin(), device_table->devices.end(),
				        [dc] (const device &dev)
				        { return !strcasecmp(dev.model.c_str(), dc->model); })) {
				// we already have this model but a different deviceid
				std::string simpleNick(dc->model);
				if (dc->deviceid == 0)
					simpleNick += " (unknown deviceid)";
				else
					simpleNick += " (" + QString::number(dc->deviceid, 16).toStdString() + ")";
				addDC(device_table->devices, dc->model, dc->deviceid, simpleNick, {}, {});
			} else {
				addDC(device_table->devices, dc->model, dc->deviceid, {}, {}, {});
			}
		}
	}
}

const char *get_dc_nickname(const struct divecomputer *dc)
{
	const device *existNode = get_device_for_dc(&device_table, dc);

	if (existNode && !existNode->nickName.empty())
		return existNode->nickName.c_str();
	else
		return dc->model;
}

extern "C" int nr_devices(const struct device_table *table)
{
	return (int)table->devices.size();
}

extern "C" const struct device *get_device(const struct device_table *table, int i)
{
	if (i < 0 || i > nr_devices(table))
		return NULL;
	return &table->devices[i];
}

extern "C" const char *device_get_model(const struct device *dev)
{
	return dev ? dev->model.c_str() : NULL;
}

extern "C" const uint32_t device_get_id(const struct device *dev)
{
	return dev ? dev->deviceId : -1;
}

extern "C" const char *device_get_serial(const struct device *dev)
{
	return dev ? dev->serialNumber.c_str() : NULL;
}

extern "C" const char *device_get_firmware(const struct device *dev)
{
	return dev ? dev->firmware.c_str() : NULL;
}

extern "C" const char *device_get_nickname(const struct device *dev)
{
	return dev ? dev->nickName.c_str() : NULL;
}

extern "C" struct device_table *alloc_device_table()
{
	return new struct device_table;
}

extern "C" void free_device_table(struct device_table *devices)
{
	delete devices;
}
