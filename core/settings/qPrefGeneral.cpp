// SPDX-License-Identifier: GPL-2.0
#include "qPrefGeneral.h"
#include "qPrefPrivate.h"

static const QString group = QStringLiteral("GeneralSettings");

QString qPrefGeneral::st_diveshareExport_uid;
static const QString st_diveshareExport_uid_default;

bool qPrefGeneral::st_diveshareExport_private;
static const bool st_diveshareExport_private_default = false;

qPrefGeneral *qPrefGeneral::instance()
{
	static qPrefGeneral *self = new qPrefGeneral;
	return self;
}

void qPrefGeneral::loadSync(bool doSync)
{
	disk_defaultsetpoint(doSync);
	disk_o2consumption(doSync);
	disk_pscr_ratio(doSync);

	if (!doSync) {
		load_diveshareExport_uid();
		load_diveshareExport_private();
	}
}

HANDLE_PREFERENCE_INT(General, "defaultsetpoint", defaultsetpoint);

HANDLE_PREFERENCE_INT(General, "o2consumption", o2consumption);

HANDLE_PREFERENCE_INT(General, "pscr_ratio", pscr_ratio);

HANDLE_PROP_QSTRING(General, "diveshareExport/uid", diveshareExport_uid);

HANDLE_PROP_BOOL(General, "diveshareExport/private", diveshareExport_private);
