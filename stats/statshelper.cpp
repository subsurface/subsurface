// SPDX-License-Identifier: GPL-2.0

#include "statshelper.h"
#include "core/dive.h"

#include <cmath>

QPointF roundPos(const QPointF &p)
{
	return QPointF(round(p.x()), round(p.y()));
}

bool allDivesSelected(const std::vector<dive *> &dives)
{
	return std::all_of(dives.begin(), dives.end(),
			   [] (const dive *d) { return d->selected; });
}
