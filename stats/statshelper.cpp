// SPDX-License-Identifier: GPL-2.0

#include "statshelper.h"

#include <cmath>

QPointF roundPos(const QPointF &p)
{
	return QPointF(round(p.x()), round(p.y()));
}
