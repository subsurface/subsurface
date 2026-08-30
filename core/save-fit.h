// SPDX-License-Identifier: GPL-2.0
// AI-generated (Claude)
#ifndef SAVE_FIT_H
#define SAVE_FIT_H

#include <cstdint>
#include <string>

struct dive;
struct divecomputer;
struct membuffer;

/*
 * Encode `dive` into a Garmin-family FIT activity file, appended to `b`.
 * Wire-format facts (message/field numbers, base types, CRC table) come only
 * from non-Garmin references (tormoder/fit, muktihari/fit Go SDKs) and the
 * in-tree GPL libdivecomputer Garmin parser -- never the Garmin FIT SDK (R009).
 *
 * Returns 0 on success. On error (no dive computer, or no samples) returns
 * non-zero and leaves `b` untouched so no partial file is produced (R010).
 */
int save_fit_to_buffer(const struct dive &dive, int tz_offset_seconds, uint16_t manufacturer_id, struct membuffer *b);

/*
 * Pure DC-vendor -> FIT manufacturer id mapping (case-insensitive).
 * Returns the FIT "invalid" uint16 sentinel (0xFFFF) for vendors with no
 * known FIT manufacturer id -- never falls back to a false vendor id.
 */
uint16_t fit_manufacturer_id(const std::string &vendor);

/*
 * Resolve the default tz offset (seconds east of UTC) to pre-fill for a FIT
 * export. Always 0: FIT timestamps carry no zone, so the correct default is
 * dive.when unshifted. `dc` is kept as a parameter since this is the single
 * documented place the default lives, but it is not read.
 */
int fit_resolve_tz_offset(const struct divecomputer &dc);

#endif
