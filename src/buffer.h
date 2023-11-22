#ifndef _BUFFER_H
#define _BUFFER_H

#include <stdio.h>

struct Buffer {
	char *data;
	size_t size;
};

enum BufferError {
	BUF_FILE_ACCESS_ERR = -3,
	BUF_FILE_READ_ERR	= -2,
	BUF_NO_MEM_ERR		= -1,
	BUF_NO_ERR			= 0,
};

enum BufferError buffer_ctor(struct Buffer *buf, const char *filename);
void buffer_dtor(struct Buffer *buf);
enum BufferError get_file_size(FILE *file, size_t *size);

#endif /*_BUFFER_H*/
