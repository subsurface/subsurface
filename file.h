#ifndef FILE_H
#define FILE_H

struct memblock {
	void *buffer;
	size_t size;
};

extern int try_to_open_cochran(const char *filename, struct memblock *mem);
extern int try_to_open_liquivision(const char *filename, struct memblock *mem);

#ifdef __cplusplus
extern "C" {
#endif
extern int readfile(const char *filename, struct memblock *mem);
extern timestamp_t parse_date(const char *date);
#ifdef __cplusplus
}
#endif

#endif // FILE_H
