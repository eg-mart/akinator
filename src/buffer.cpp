#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <sys/stat.h>

#include "buffer.h"

enum BufferError buffer_ctor(struct Buffer *buf, const char *filename)
{
	assert(buf);
	assert(filename);

	FILE *input = fopen(filename, "r");
	if (!input)
		return BUF_FILE_ACCESS_ERR;

	size_t filesize = 0;
	enum BufferError err = get_file_size(input, &filesize);
	if (err < 0)
		return err;
	buf->size = filesize + 1;

	buf->data = (char*) calloc(buf->size, sizeof(char));
	if (!buf->data)
		return BUF_NO_MEM_ERR;
	size_t read_chars = fread(buf->data, sizeof(char), buf->size - 1, input);
	if (read_chars < buf->size - 1)
		return BUF_FILE_READ_ERR;

	return BUF_NO_ERR;
}

void buffer_dtor(struct Buffer *buf)
{
	buf->size = 0;
	free(buf->data);
	buf->data = NULL;
}

enum BufferError get_file_size(FILE *file, size_t *size)
{
	assert(file);
	assert(size);

	int fd = fileno(file);
	struct stat stbuf;
	if (fstat(fd, &stbuf) == -1) return BUF_FILE_ACCESS_ERR;
	*size = (size_t) stbuf.st_size;
	return BUF_NO_ERR;
}
