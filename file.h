#ifndef FILE_H
#define FILE_H

struct memblock {
	void *buffer;
	size_t size;
};

#if 0
extern int try_to_open_cochran(const char *filename, struct memblock *mem, GError **error);
#endif
extern int readfile(const char *filename, struct memblock *mem);

#endif
