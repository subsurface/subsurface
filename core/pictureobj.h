// SPDX-License-Identifier: GPL-2.0

#ifndef PICTUREOBJ_H
#define PICTUREOBJ_H

// A tiny helper class that represents a struct picture of the core
// It does, however, keep the filename as a std::string so that C++ code
// doesn't have do its own memory-management.

#include "core/units.h"
#include "core/picture.h"
#include <string>

struct PictureObj {
	std::string filename;
	offset_t offset;
	location_t location;

	PictureObj();			// Initialize to empty picture.
	PictureObj(const picture &pic); // Create from core struct picture.
	picture toCore() const;		// Turn into core structure. Caller responsible for freeing.
	bool operator<(const PictureObj &p2) const;
};

#endif // PICTUREOBJ_H
