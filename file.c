#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <errno.h>

#include "dive.h"

struct memblock {
	void *buffer;
	size_t size;
};

static int readfile(const char *filename, struct memblock *mem)
{
	int ret, fd = open(filename, O_RDONLY);
	struct stat st;

	mem->buffer = NULL;
	mem->size = 0;

	fd = open(filename, O_RDONLY);
	if (fd < 0)
		return fd;
	ret = fstat(fd, &st);
	if (ret < 0)
		goto out;
	ret = -EINVAL;
	if (!S_ISREG(st.st_mode))
		goto out;
	ret = 0;
	if (!st.st_size)
		goto out;
	mem->buffer = malloc(st.st_size);
	ret = -1;
	errno = ENOMEM;
	if (!mem->buffer)
		goto out;
	mem->size = st.st_size;
	ret = read(fd, mem->buffer, mem->size);
	if (ret < 0)
		goto free;
	if (ret == mem->size)
		goto out;
	errno = EIO;
	ret = -1;
free:
	free(mem->buffer);
	mem->buffer = NULL;
	mem->size = 0;
out:
	close(fd);
	return ret;
}

void parse_file(const char *filename, GError **error)
{
	struct memblock mem;

	if (readfile(filename, &mem) < 0) {
		fprintf(stderr, "Failed to read '%s'.\n", filename);
		if (error) {
			*error = g_error_new(g_quark_from_string("subsurface"),
					     DIVE_ERROR_PARSE,
					     "Failed to read '%s'",
					     filename);
		}
		return;
	}

	parse_xml_buffer(filename, mem.buffer, mem.size, error);
	free(mem.buffer);
}
