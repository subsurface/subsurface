// SPDX-License-Identifier: GPL-2.0
// Helper functions to render the stats.
#ifndef STATSHELPER_H
#define STATSHELPER_H

#include <vector>
#include <QPointF>

struct dive;

// Round positions to integer values to avoid ugly artifacts
QPointF roundPos(const QPointF &p);

// Are all dives in this vector selected?
bool allDivesSelected(const std::vector<dive *> &dives);

#endif
