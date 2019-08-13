// SPDX-License-Identifier: GPL-2.0
#include "CylinderObjectHelper.h"
#include "../qthelper.h"

static QString EMPTY_CYLINDER_STRING = QStringLiteral("");
CylinderObjectHelper::CylinderObjectHelper(const cylinder_t *cylinder)
{
	if (!cylinder)
		return;

	description = cylinder->type.description ? cylinder->type.description:
						   EMPTY_CYLINDER_STRING;
	size = get_volume_string(cylinder->type.size, true);
	workingPressure = get_pressure_string(cylinder->type.workingpressure, true);
	startPressure = get_pressure_string(cylinder->start, true);
	endPressure = get_pressure_string(cylinder->end, true);
	gasMix = get_gas_string(cylinder->gasmix);
}
