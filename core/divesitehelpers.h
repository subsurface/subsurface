// SPDX-License-Identifier: GPL-2.0
#ifndef DIVESITEHELPERS_H
#define DIVESITEHELPERS_H

#include "taxonomy.h"
#include "units.h"

/// Performs a reverse geo-lookup and returns the data.
/// It is up to the caller to merge the data with any existing data.
taxonomy_data reverseGeoLookup(degrees_t latitude, degrees_t longitude);

#endif // DIVESITEHELPERS_H
