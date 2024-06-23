// SPDX-License-Identifier: GPL-2.0
#ifndef DIVEMODE_H
#define DIVEMODE_H

enum divemode_t {OC, CCR, PSCR, FREEDIVE, NUM_DIVEMODE, UNDEF_COMP_TYPE};	// Flags (Open-circuit and Closed-circuit-rebreather) for setting dive computer type

#define IS_REBREATHER_MODE(divemode) ((divemode) == CCR || (divemode) == PSCR)

#endif
