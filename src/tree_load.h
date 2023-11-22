#ifndef _TREE_LOAD_H
#define _TREE_LOAD_H

#include <stdio.h>

#include "tree.h"

struct Buffer {
	char *data;
	size_t size;
};

enum LoadError {
	LOAD_TREE_ERR	= -5,
	LOAD_NO_MEM_ERR = -4,
	LOAD_FILE_ACCESS_ERR = -3,
	LOAD_FILE_READ_ERR = -2,
	LOAD_SYNTAX_ERR = -1,
	LOAD_NO_ERR = 0,
};

enum LoadError buffer_ctor(struct Buffer *buf, const char *filename);
void buffer_dtor(struct Buffer *buf);

enum LoadError tree_load(struct Node **tree, struct Buffer *buf);
enum LoadError get_file_size(FILE *file, size_t *size);

#endif /*_TREE_LOAD_H*/
