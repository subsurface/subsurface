// SPDX-License-Identifier: GPL-2.0
#include "metadata.h"
#include "exif.h"
#include "qthelper.h"
#include <QString>
#include <QFile>
#include <QDateTime>

// Fetch quint16 in big endian mode from QFile and return 0 on error.
// This is a very specialized function for parsing JPEGs, therefore we can get away with such an in-band error code.
static inline quint16 getShortBE(QFile &f)
{
	unsigned char buf[2];
	if (f.read(reinterpret_cast<char *>(buf), 2) != 2)
		return 0;
	return (buf[0] << 8) | buf[1];
}

static bool parseExif(QFile &f, struct metadata *metadata)
{
	if (getShortBE(f) != 0xffd8)
		return false;
	for (;;) {
		switch (getShortBE(f)) {
		case 0xffc0:
		case 0xffc2:
		case 0xffc4:
		case 0xffd0 ... 0xffd7:
		case 0xffdb:
		case 0xffdd:
		case 0xffe0:
		case 0xffe2 ... 0xffef:
		case 0xfffe: {
			quint16 len = getShortBE(f);
			if (len < 2)
				return false;
			f.seek(f.pos() + len - 2); // TODO: switch to QFile::skip()
			break;
		}
		case 0xffe1: {
			quint16 len = getShortBE(f);
			if (len < 2)
				return false;
			len -= 2;
			QByteArray data = f.read(len);
			if (data.size() != len)
				return false;
			easyexif::EXIFInfo exif;
			if (exif.parseFromEXIFSegment(reinterpret_cast<const unsigned char *>(data.constData()), len) != PARSE_EXIF_SUCCESS)
				return false;
			metadata->longitude.udeg = lrint(1000000.0 * exif.GeoLocation.Longitude);
			metadata->latitude.udeg = lrint(1000000.0 * exif.GeoLocation.Latitude);
			metadata->timestamp = exif.epoch();
			return true;
		}
		case 0xffda:
		case 0xffd9:
			// We expect EXIF data before any scan data
			return false;
		default:
			return false;
		}
	}
}

static bool parseMP4(QFile &, metadata *)
{
	// TODO: Implement MP4 parsing
	return false;
}

extern "C" mediatype_t get_metadata(const char *filename_in, metadata *data)
{
	data->timestamp = 0;
	data->latitude.udeg = 0;
	data->longitude.udeg = 0;

	QString filename = localFilePath(QString(filename_in));
	QFile f(filename);
	if (!f.open(QIODevice::ReadOnly))
		return MEDIATYPE_IO_ERROR;

	if (parseExif(f, data)) {
		return MEDIATYPE_PICTURE;
	} else if(parseMP4(f, data)) {
		return MEDIATYPE_VIDEO;
	} else {
		// If we couldn't parse EXIF or MP4 data, use file creation date.
		// TODO: QFileInfo::created is deprecated in newer Qt versions.
		data->timestamp = QFileInfo(filename).created().toMSecsSinceEpoch() / 1000;
		return MEDIATYPE_UNKNOWN;
	}
}

extern "C" timestamp_t picture_get_timestamp(const char *filename)
{
	struct metadata data;
	get_metadata(filename, &data);
	return data.timestamp;
}

extern "C" void picture_load_exif_data(struct picture *p)
{
	struct metadata data;
	if (get_metadata(p->filename, &data) == MEDIATYPE_IO_ERROR)
		return;
	p->longitude = data.longitude;
	p->latitude = data.latitude;
}
