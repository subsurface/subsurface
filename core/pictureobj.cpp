// SPDX-License-Identifier: GPL-2.0
#include "pictureobj.h"
#include "qthelper.h"

PictureObj::PictureObj() : offset({ 0 }), location({ 0 })
{
}

PictureObj::PictureObj(const picture &pic) : filename(pic.filename), offset(pic.offset), location(pic.location)
{
}

picture PictureObj::toCore() const
{
	return picture {
		strdup(filename.c_str()),
		offset,
		location
	};
}

bool PictureObj::operator<(const PictureObj &p2) const
{
	if (offset.seconds != p2.offset.seconds)
		return offset.seconds < p2.offset.seconds;
	return strcmp(filename.c_str(), p2.filename.c_str()) < 0;
}
