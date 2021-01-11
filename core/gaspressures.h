// SPDX-License-Identifier: GPL-2.0
#ifndef GASPRESSURES_H
#define GASPRESSURES_H

#ifdef __cplusplus
extern "C" {
#endif

void populate_pressure_information(const struct dive *, const struct divecomputer *, struct plot_info *, int);

#ifdef __cplusplus
}
#endif
#endif // GASPRESSURES_H
