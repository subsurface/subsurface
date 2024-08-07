// SPDX-License-Identifier: GPL-2.0
#include "picture.h"
#include "dive.h"
#include "divelist.h"
#include "divelog.h"
#if !defined(SUBSURFACE_MOBILE)
#include "metadata.h"
#endif
#include "range.h"
#include "subsurface-string.h"
#include <stdlib.h>
#include <string.h>

bool picture::operator<(const struct picture &p) const
{
	return std::tie(offset.seconds, filename) <
	       std::tie(p.offset.seconds, p.filename);
}

void add_picture(picture_table &t, struct picture newpic)
{
	auto it = std::lower_bound(t.begin(), t.end(), newpic);
	t.insert(it, std::move(newpic));
}

int get_picture_idx(const picture_table &t, const std::string &filename)
{

	return index_of_if(t, [&filename] (const picture &p)
			   { return p.filename == filename; });
}

#if !defined(SUBSURFACE_MOBILE)
/* Return distance of timestamp to time of dive. Result is always positive, 0 means during dive. */
static timestamp_t time_from_dive(const struct dive &d, timestamp_t timestamp)
{
	timestamp_t end_time = d.endtime();
	if (timestamp < d.when)
		return d.when - timestamp;
	else if (timestamp > end_time)
		return timestamp - end_time;
	else
		return 0;
}

/* Return dive closest selected dive to given timestamp or NULL if no dives are selected. */
static struct dive *nearest_selected_dive(timestamp_t timestamp)
{
	struct dive *res = NULL;
	timestamp_t min = 0;

	for (auto &d: divelog.dives) {
		if (!d->selected)
			continue;
		timestamp_t offset = time_from_dive(*d, timestamp);
		if (!res || offset < min) {
			res = d.get();
			min = offset;
		}

		/* We suppose that dives are sorted chronologically. Thus
		 * if the offset starts to increase, we can end. This ignores
		 * pathological cases such as overlapping dives. In such a
		 * case the user will have to add pictures manually.
		 */
		if (offset == 0 || offset > min)
			break;
	}
	return res;
}

// only add pictures that have timestamps between 30 minutes before the dive and
// 30 minutes after the dive ends
static constexpr timestamp_t d30min = 30 * 60;
static bool dive_check_picture_time(const struct dive &d, timestamp_t timestamp)
{
	return time_from_dive(d, timestamp) < d30min;
}

/* Creates a picture and indicates the dive to which this picture should be added.
 * The caller is responsible for actually adding the picture to the dive.
 * If no appropriate dive was found, no picture is created and null is returned.
 */
std::pair<std::optional<picture>, dive *> create_picture(const std::string &filename, timestamp_t shift_time, bool match_all)
{
	struct metadata metadata;
	timestamp_t timestamp;

	get_metadata(filename.c_str(), &metadata);
	timestamp = metadata.timestamp + shift_time;
	struct dive *dive = nearest_selected_dive(timestamp);

	if (!dive)
		return { {}, nullptr };
	if (get_picture_idx(dive->pictures, filename) >= 0)
		return { {}, nullptr };
	if (!match_all && !dive_check_picture_time(*dive, timestamp))
		return { {}, nullptr };

	struct picture picture;
	picture.filename = filename;
	picture.offset.seconds = metadata.timestamp - dive->when + shift_time;
	picture.location = metadata.location;
	return { picture, dive };
}

bool picture_check_valid_time(timestamp_t timestamp, timestamp_t shift_time)
{
	return std::any_of(divelog.dives.begin(), divelog.dives.end(),
			   [t = timestamp + shift_time] (auto &d)
			   { return d->selected && dive_check_picture_time(*d, t); });
}
#endif
