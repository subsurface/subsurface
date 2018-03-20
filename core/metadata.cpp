// SPDX-License-Identifier: GPL-2.0
#include "metadata.h"
#include "exif.h"
#include "qthelper.h"
#include <QString>
#include <QFile>
#include <QDateTime>

// Weirdly, android builds fail owing to undefined UINT64_MAX
#ifndef UINT64_MAX
#define UINT64_MAX (~0ULL)
#endif

// The following two functions fetch an arbitrary-length _unsigned_ integer from either
// a file or a memory location in big-endian mode. The size of the integer is passed
// via a template argument [e.g. getBE<uint16_t>(...)].
// The function doing file access returns a default value on IO error or end-of-file.
// Warning: This code works properly only for unsigned integers. The template parameter
// is not checked and passing a signed integer will silently fail!
template <typename T>
static inline T getBE(const char *buf_in)
{
	constexpr size_t size = sizeof(T);
	// Interpret raw bytes as unsigned char to avoid sign extension for
	// characters in the 0x80...0xff range.
	auto buf = (unsigned const char *)buf_in;
	T ret = 0;
	for (size_t i = 0; i < size; ++i)
		ret = (ret << 8) | buf[i];
	return ret;
}

template <typename T>
static inline T getBE(QFile &f, T def=0)
{
	constexpr size_t size = sizeof(T);
	char buf[size];
	if (f.read(buf, size) != size)
		return def;
	return getBE<T>(buf);
}

static bool parseExif(QFile &f, struct metadata *metadata)
{
	f.seek(0);
	if (getBE<uint16_t>(f) != 0xffd8)
		return false;
	for (;;) {
		switch (getBE<uint16_t>(f)) {
		case 0xffc0:
		case 0xffc2:
		case 0xffc4:
		case 0xffd0 ... 0xffd7:
		case 0xffdb:
		case 0xffdd:
		case 0xffe0:
		case 0xffe2 ... 0xffef:
		case 0xfffe: {
			uint16_t len = getBE<uint16_t>(f);
			if (len < 2)
				return false;
			f.seek(f.pos() + len - 2); // TODO: switch to QFile::skip()
			break;
		}
		case 0xffe1: {
			uint16_t len = getBE<uint16_t>(f);
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

static bool parseMP4(QFile &f, metadata *metadata)
{
	f.seek(0);

	// MP4s and related formats are hierarchical, being made up of "atoms", which can
	// contain other atoms (an interesting interpretation of the term atom).
	// To parse the file, the remaining to-be-parsed bytes of the upper atoms in
	// the parse-tree are tracked in a stack-like structure. This is not strictly
	// necessary, since the level at which an atom is found is insubstantial.
	// Nevertheless, it is an effective and simple way of sanity-checking the file and the
	// parsing routine.
	std::vector<uint64_t> atom_stack;
	atom_stack.reserve(10);

	// For the outmost level, set the atom-size the the maximum value representable in
	// 64-bits, which effectively means parse to the end of file.
	atom_stack.push_back(UINT64_MAX);

	// The first atom of an MP4 or related video is supposed to be of the "ftyp" kind.
	// If such an atom is found as first atom, this function will return true, indicating
	// that the file is a video.
	bool found_ftyp = false;

	while (!f.atEnd() && !atom_stack.empty()) {
		// Parse atom header. The header can have two forms (each character stands for a byte):
		//	lllltttt
		// or
		// 	0001ttttllllllll
		// where "l" stands for length in big-endian mode and "t" for type of the atom.
		// The length includes the 8- or 16-bytes header.
		uint64_t atom_size = getBE<uint32_t>(f, 2);
		int atom_header_size = 8;
		if (atom_size > 1 && atom_size < 8)
			break;
		char type[4];
		if (f.read(type, 4) != 4)
			break;
		if (atom_size == 1) {
			atom_size = getBE<uint64_t>(f);
			atom_header_size = 16;
			if (atom_size < 16)
			       break;
		}
		if (atom_size == 0)
			atom_size = atom_stack.back();
		if (atom_size > atom_stack.back())
			break;
		atom_stack.back() -= atom_size;
		atom_size -= atom_header_size;

		// The first atom must be "ftyp"
		if (!found_ftyp) {
			found_ftyp = !memcmp(type, "ftyp", 4);
			if (!found_ftyp)
				break;
		}

		if (!memcmp(type, "moov", 4) ||
		    !memcmp(type, "trak", 4) ||
		    !memcmp(type, "mdia", 4)) {
			// Recurse into "moov", "trak" and "mdia" atoms
			atom_stack.push_back(atom_size);
			continue;
		} else if (!memcmp(type, "mdhd", 4) && atom_size >= 24 && atom_size < 4096) {
			// Parse "mdhd" (media header).
			// Sanity check: size between 24 and 4096
			std::vector<char> data(atom_size);
			if (f.read(&data[0], atom_size) != static_cast<int>(atom_size))
				break;
			uint64_t timestamp = 0;
			// First byte is version. We know version 0 and 1
			switch (data[0]) {
			case 0:
				timestamp = getBE<uint32_t>(&data[4]);
				break;
			case 1:
				timestamp = getBE<uint64_t>(&data[4]);
				break;
			default:
				// For unknown versions: ignore -> maybe we find a parseable "mdhd" atom later in this file
				break;
			}
			// Timestamp is given as seconds since midnight 1904/1/1. To be convertible to the UNIX epoch
			// it must be larger than 2082844800.
			if (timestamp >= 2082844800) {
				metadata->timestamp = timestamp - 2082844800;
				// Currently, we only know how to extract timestamps, so we might just quit parsing here.
				break;
			}
		} else {
			// Jump over unknown atom
			if (!f.seek(f.pos() + atom_size)) // TODO: switch to QFile::skip()
				break;
		}

		// If end of atom is reached, return to outer atom
		while (!atom_stack.empty() && atom_stack.back() == 0)
			atom_stack.pop_back();
	}

	return found_ftyp;
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
