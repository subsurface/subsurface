#ifndef METADATA_H
#define METADATA_H

#include "units.h"

struct metadata {
	timestamp_t	timestamp;
	duration_t	duration;
	location_t	location;
};

enum mediatype_t {
	MEDIATYPE_UNKNOWN,		// Couldn't (yet) identify file
	MEDIATYPE_IO_ERROR,		// Couldn't read file
	MEDIATYPE_PICTURE,
	MEDIATYPE_VIDEO,
	MEDIATYPE_STILL_LOADING,	// Still processing in the background
};

enum mediatype_t get_metadata(const char *filename, struct metadata *data);
timestamp_t picture_get_timestamp(const char *filename);

#endif // METADATA_H
