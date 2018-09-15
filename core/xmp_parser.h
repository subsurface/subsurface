// Parse XMP blocks using libxml2

#ifndef XMP_PARSER_H
#define XMP_PARSER_H

#include "units.h" // for timestamp_t
#include <stddef.h> // for size_t

timestamp_t parse_xmp(const char *data, size_t size); // On failure returns 0.

#endif // XMP_PARSER_H
