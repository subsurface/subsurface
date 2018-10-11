// SPDX-License-Identifier: GPL-2.0
#ifndef DIVESITEHELPERS_H
#define DIVESITEHELPERS_H

#include "taxonomy.h"
#include "units.h"

// Perform a reverse geo-lookup and put data in the provided taxonomy field.
// Original data with the exception of OCEAN will be overwritten.
void reverseGeoLookup(degrees_t latitude, degrees_t longitude, taxonomy_data *taxonomy);

#endif // DIVESITEHELPERS_H
