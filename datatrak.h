#ifndef DATATRAK_HEADER_H
#define DATATRAK_HEADER_H

#include <string.h>

typedef struct dtrakheader_ {
	int header; //Must be 0xA100;
	int divesNum;
	int dc_serial_1;
	int dc_serial_2;
} dtrakheader;

#define read_bytes(_n) \
	switch (_n) { \
		case 1: \
			if (fread (&lector_bytes, sizeof(char), _n, archivo) != _n) \
				goto bail; \
			tmp_1byte = lector_bytes[0]; \
			break; \
		case 2: \
			if (fread (&lector_bytes, sizeof(char), _n, archivo) != _n) \
				goto bail; \
			tmp_2bytes = two_bytes_to_int (lector_bytes[1], lector_bytes[0]); \
			break; \
		default: \
			if (fread (&lector_word, sizeof(char), _n, archivo) != _n) \
				goto bail; \
			tmp_4bytes = four_bytes_to_long(lector_word[3], lector_word[2], lector_word[1], lector_word[0]); \
			break; \
	}

#define read_string(_property) \
	unsigned char *_property##tmp = (unsigned char *)calloc(tmp_1byte + 1, 1); \
	if (fread((char *)_property##tmp, 1, tmp_1byte, archivo) != tmp_1byte) \
		goto bail; \
	_property = (unsigned char *)strcat(to_utf8(_property##tmp), ""); \
	free(_property##tmp);

#endif // DATATRAK_HEADER_H
