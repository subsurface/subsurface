// SPDX-License-Identifier: GPL-2.0
#include "divepixmapcache.h"
#include "core/errorhelper.h"
#include "core/metrics.h"
#include "core/color.h"

#include <cmath>

DivePixmaps::~DivePixmaps()
{
}

static QPixmap createPixmap(const char *name, int size)
{
	return QPixmap(QString(name)).scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

DivePixmaps::DivePixmaps(int dpr) : dpr(dpr)
{
	extern int verbose;
	double dprf = dpr / 100.0;
	const IconMetrics &metrics = defaultIconMetrics();
	int sz_bigger = metrics.sz_med + metrics.sz_small; // ex 40px
	sz_bigger = lrint(sz_bigger * dprf);
	int sz_pix = sz_bigger / 2; // ex 20px
	if (verbose)
		report_info("%s DPR: %f metrics: %d %d sz_bigger: %d", __func__, dprf, metrics.sz_med, metrics.sz_small, sz_bigger);

	warning = createPixmap(":status-warning-icon", sz_pix);
	info = createPixmap(":status-info-icon", sz_pix);
	violation = createPixmap(":status-violation-icon", sz_pix);
	bailout = createPixmap(":bailout-icon", sz_pix);
	onCCRLoop = createPixmap(":onCCRLoop-icon", sz_pix);
	bookmark = createPixmap(":dive-bookmark-icon", sz_pix);
	gaschangeTrimixICD = createPixmap(":gaschange-trimix-ICD-icon", sz_bigger);
	gaschangeTrimix = createPixmap(":gaschange-trimix-icon", sz_bigger);
	gaschangeAirICD = createPixmap(":gaschange-air-ICD-icon", sz_bigger);
	gaschangeAir = createPixmap(":gaschange-air-icon", sz_bigger);
	gaschangeOxygenICD = createPixmap(":gaschange-oxygen-ICD-icon", sz_bigger);
	gaschangeOxygen = createPixmap(":gaschange-oxygen-icon", sz_bigger);
	gaschangeEANICD = createPixmap(":gaschange-ean-ICD-icon", sz_bigger);
	gaschangeEAN = createPixmap(":gaschange-ean-icon", sz_bigger);

	// The transparent pixmap is a very obscure feature to enable tooltips without showing a pixmap.
	// See code in diveeventitem.cpp. This should probably be replaced by a different mechanism.
	QPixmap transparentPixmap(lrint(4 * dprf), lrint(20 * dprf));
	transparentPixmap.fill(makeColor(1.0, 1.0, 1.0, 0.01));
}

static std::vector<std::shared_ptr<const DivePixmaps>> cache;

// Return a std::shared_ptr<> for reference counting.
// Note that the idiomatic way would be to store std::weak_ptr<>s.
// That would mean that the pixmaps are destroyed when the last user uses
// them. We want to keep them around at least until the next caller!
// Therefore we also store std::shared_ptr<>s.
std::shared_ptr<const DivePixmaps> getDivePixmaps(double dprIn)
{
	using ptr = std::shared_ptr<const DivePixmaps>;
	ptr res;
	int dpr = lrint(dprIn * 100.0);		// Caching on a percent basis should be fine.
	auto it = std::find_if(cache.begin(), cache.end(), [dpr](const ptr &p) { return p->dpr == dpr; });
	if (it == cache.end()) {
		res = std::make_shared<DivePixmaps>(dpr);
		cache.push_back(res);
	} else {
		res = *it;
	}

	// Remove unused items with C++'s wonderful erase/remove idiom.
	// If the use_count is one, then the cache has the only reference, so remove the object.
	cache.erase(std::remove_if(cache.begin(), cache.end(),
				   [](const ptr &p) { return p.use_count() <= 1; }), cache.end());

	return res;
}
