#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/stat.h>

#include "tree_load.h"

static enum LoadError _tree_load(struct Node **tree, char **buf);
static char *skip_space(char *str);
static size_t get_word_len(const char *str);
static enum LoadError mark_between_brackets(char **buf);

enum LoadError buffer_ctor(struct Buffer *buf, const char *filename)
{
	assert(buf);
	assert(filename);

	FILE *input = fopen(filename, "r");
	if (!input)
		return LOAD_FILE_ACCESS_ERR;

	size_t filesize = 0;
	enum LoadError err = get_file_size(input, &filesize);
	if (err < 0)
		return err;
	buf->size = filesize + 1;

	buf->data = (char*) calloc(buf->size, sizeof(char));
	if (!buf->data)
		return LOAD_NO_MEM_ERR;
	size_t read_chars = fread(buf->data, sizeof(char), buf->size - 1, input);
	if (read_chars < buf->size - 1)
		return LOAD_FILE_READ_ERR;

	return LOAD_NO_ERR;
}

void buffer_dtor(struct Buffer *buf)
{
	assert(buf);

	buf->size = 0;
	free(buf->data);
	buf->data = NULL;
}

enum LoadError tree_load(struct Node **tree, struct Buffer *buf)
{
	assert(tree);
	assert(buf);

	char *data_iter = buf->data;
	return _tree_load(tree, &data_iter);
}

static char *skip_space(char *str)
{
	assert(str);

	while (isspace(*str))
		str++;
	return str;
}

static size_t get_word_len(const char *str)
{
	size_t len = 0;
	if (!isalpha(str[0]) && str[0] != '_')
		return 1;
	
	while (str[len] != '\0' && (isalnum(str[len]) || str[len] == '_'))
		len++;
	return len;
}

static enum LoadError mark_between_brackets(char **buf)
{
	assert(buf);
	assert(*buf);

	if (**buf != '<')
		return LOAD_SYNTAX_ERR;
	(*buf)++;
	char *iter = *buf;
	while (*iter && *iter != '>')
		iter++;
	if (*iter != '>')
		return LOAD_SYNTAX_ERR;
	*iter = '\0';
	return LOAD_NO_ERR;
}

static enum LoadError _tree_load(struct Node **tree, char **buf)
{
	assert(tree);
	assert(buf);
	assert(*buf);

	*buf = skip_space(*buf);
	size_t word_len = get_word_len(*buf);

	if (strncmp(*buf, "nil", word_len) == 0 && word_len == strlen("nil")) {
		*buf = skip_space(*buf + word_len);
		*tree = NULL;
		return LOAD_NO_ERR;
	}
	if (strncmp(*buf, "(", word_len) == 0 && word_len == strlen("("))  {
		*buf = skip_space(*buf + word_len);
		enum LoadError ld_err = mark_between_brackets(buf);
		if (ld_err < 0)
			return ld_err;
		enum TreeError tr_err = node_op_new(tree, *buf);
		if (tr_err < 0)
			return LOAD_TREE_ERR;
		*buf = *buf + strlen(*buf) + 1;
		ld_err = _tree_load(&(*tree)->left, buf);
		if (ld_err < 0)
			return ld_err;
		ld_err = _tree_load(&(*tree)->right, buf);
		if (ld_err < 0)
			return ld_err;
		*buf = skip_space(*buf);
		if (*buf[0] != ')')
			return LOAD_SYNTAX_ERR;
		(*buf)++;
		return LOAD_NO_ERR;
	}
	return LOAD_SYNTAX_ERR;
}

enum LoadError get_file_size(FILE *file, size_t *size)
{
	assert(file);
	assert(size);

	int fd = fileno(file);
	struct stat stbuf;
	if (fstat(fd, &stbuf) == -1) return LOAD_FILE_ACCESS_ERR;
	*size = (size_t) stbuf.st_size;
	return LOAD_NO_ERR;
}