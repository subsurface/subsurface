// SPDX-License-Identifier: GPL-2.0
#include "qPrefPartialPressureGas.h"
#include "qPrefPrivate.h"

static const QString group = QStringLiteral("TecDetails");

qPrefPartialPressureGas *qPrefPartialPressureGas::instance()
{
	static qPrefPartialPressureGas *self = new qPrefPartialPressureGas;
	return self;
}

void qPrefPartialPressureGas::loadSync(bool doSync)
{
	disk_phe(doSync);
	disk_phe_threshold(doSync);
	disk_pn2(doSync);
	disk_pn2_threshold(doSync);
	disk_po2(doSync);
	disk_po2_threshold_min(doSync);
	disk_po2_threshold_max(doSync);
}


HANDLE_PREFERENCE_BOOL_EXT(PartialPressureGas, "phegraph", phe, pp_graphs.);

HANDLE_PREFERENCE_DOUBLE_EXT(PartialPressureGas, "phethreshold", phe_threshold, pp_graphs.);

HANDLE_PREFERENCE_BOOL_EXT(PartialPressureGas, "pn2graph", pn2, pp_graphs.);

HANDLE_PREFERENCE_DOUBLE_EXT(PartialPressureGas, "pn2threshold", pn2_threshold, pp_graphs.);

HANDLE_PREFERENCE_BOOL_EXT(PartialPressureGas, "po2graph", po2, pp_graphs.);

HANDLE_PREFERENCE_DOUBLE_EXT(PartialPressureGas, "po2thresholdmax", po2_threshold_max, pp_graphs.);

HANDLE_PREFERENCE_DOUBLE_EXT(PartialPressureGas, "po2thresholdmin", po2_threshold_min, pp_graphs.);
