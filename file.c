#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "dive.h"
#include "file.h"

static int readfile(const char *filename, struct memblock *mem)
{
	int ret, fd = open(filename, O_RDONLY);
	struct stat st;
	char *buf;

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
	buf = malloc(st.st_size+1);
	ret = -1;
	errno = ENOMEM;
	if (!buf)
		goto out;
	mem->buffer = buf;
	mem->size = st.st_size;
	ret = read(fd, buf, mem->size);
	if (ret < 0)
		goto free;
	buf[ret] = 0;
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

#ifdef LIBZIP
#include <zip.h>

static void suunto_read(struct zip_file *file, GError **error)
{
	int size = 1024, n, read = 0;
	char *mem = malloc(size);

	while ((n = zip_fread(file, mem+read, size-read)) > 0) {
		read += n;
		size = read * 3 / 2;
		mem = realloc(mem, size);
	}
	parse_xml_buffer("SDE file", mem, read, error);
	free(mem);
}
#endif

static int try_to_open_suunto(const char *filename, struct memblock *mem, GError **error)
{
	int success = 0;
#ifdef LIBZIP
	/* Grr. libzip needs to re-open the file, it can't take a buffer */
	struct zip *zip = zip_open(filename, ZIP_CHECKCONS, NULL);

	if (zip) {
		int index;
		for (index = 0; ;index++) {
			struct zip_file *file = zip_fopen_index(zip, index, 0);
			if (!file)
				break;
			suunto_read(file, error);
			zip_fclose(file);
			success++;
		}
		zip_close(zip);
	}
#endif
	return success;
}

static int open_by_filename(const char *filename, const char *fmt, struct memblock *mem, GError **error)
{
	/* Suunto Dive Manager files: SDE */
	if (!strcasecmp(fmt, "SDE"))
		return try_to_open_suunto(filename, mem, error);

	/* Truly nasty intentionally obfuscated Cochran Anal software */
	if (!strcasecmp(fmt, "CAN"))
		return try_to_open_cochran(filename, mem, error);

	return 0;
}

static void parse_file_buffer(const char *filename, struct memblock *mem, GError **error)
{
	char *fmt = strrchr(filename, '.');
	if (fmt && open_by_filename(filename, fmt+1, mem, error))
		return;

	parse_xml_buffer(filename, mem->buffer, mem->size, error);
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

	parse_file_buffer(filename, &mem, error);
	free(mem.buffer);
}
