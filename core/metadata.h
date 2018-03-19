#ifndef METADATA_H
#define METADATA_H

#include "units.h"

struct metadata {
	timestamp_t	timestamp;
	degrees_t	latitude;
	degrees_t	longitude;
};

enum mediatype_t {
	MEDIATYPE_IO_ERROR,	// Couldn't read file
	MEDIATYPE_UNKNOWN,	// Couldn't identify file
	MEDIATYPE_PICTURE,
	MEDIATYPE_VIDEO,
};

#ifdef __cplusplus
extern "C" {
#endif

enum mediatype_t get_metadata(const char *filename, struct metadata *data);
timestamp_t picture_get_timestamp(const char *filename);

#ifdef __cplusplus
}
#endif

#endif // METADATA_H
