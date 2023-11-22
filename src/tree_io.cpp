#include <ctype.h>
#include <assert.h>
#include <string.h>

#include "tree_io.h"

static enum TreeIOError _tree_load(struct Node **tree, char **buf);
static char *skip_space(char *str);
static size_t get_word_len(const char *str);
static enum TreeIOError mark_between_brackets(char **buf);
void _subtree_save(const struct Node *node, FILE *out, size_t level);

enum TreeIOError tree_load_from_buf(struct Node **tree, struct Buffer *buf)
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

static enum TreeIOError mark_between_brackets(char **buf)
{
	assert(buf);
	assert(*buf);

	if (**buf != '<')
		return TRIO_SYNTAX_ERR;
	(*buf)++;
	char *iter = *buf;
	while (*iter && *iter != '>')
		iter++;
	if (*iter != '>')
		return TRIO_SYNTAX_ERR;
	*iter = '\0';
	return TRIO_NO_ERR;
}

static enum TreeIOError _tree_load(struct Node **tree, char **buf)
{
	assert(tree);
	assert(buf);
	assert(*buf);

	*buf = skip_space(*buf);
	size_t word_len = get_word_len(*buf);

	if (strncmp(*buf, "nil", word_len) == 0 && word_len == strlen("nil")) {
		*buf = skip_space(*buf + word_len);
		*tree = NULL;
		return TRIO_NO_ERR;
	}
	if (strncmp(*buf, "(", word_len) == 0 && word_len == strlen("("))  {
		*buf = skip_space(*buf + word_len);
		enum TreeIOError trio_err = mark_between_brackets(buf);
		if (trio_err < 0)
			return trio_err;
		enum TreeError tr_err = node_op_new(tree, *buf);
		if (tr_err < 0)
			return TRIO_TREE_ERR;
		*buf = *buf + strlen(*buf) + 1;
		trio_err = _tree_load(&(*tree)->left, buf);
		if (trio_err < 0)
			return trio_err;
		trio_err = _tree_load(&(*tree)->right, buf);
		if (trio_err < 0)
			return trio_err;
		*buf = skip_space(*buf);
		if (*buf[0] != ')')
			return TRIO_SYNTAX_ERR;
		(*buf)++;
		return TRIO_NO_ERR;
	}
	return TRIO_SYNTAX_ERR;
}

void tree_save(const struct Node *tree, FILE *out)
{
	assert(out);

	_subtree_save(tree, out, 0);
}

void _subtree_save(const struct Node *node, FILE *out, size_t level)
{
	assert(out);

	for (size_t i = 0; i < level; i++)
		fputs("    ", out);
	
	if (!node) {
		fputs("nil\n", out);
		return;
	}

	fprintf(out, "(<%s>\n", node->data);
	_subtree_save(node->left, out, level + 1);
	_subtree_save(node->right, out, level + 1);

	for (size_t i = 0; i < level; i++)
		fputs("    ", out);
	fputs(")\n", out);
}

const char *tree_io_err_to_str(enum TreeIOError err)
{
	switch (err) {
		case TRIO_SYNTAX_ERR:
			return "Syntax error in tree's string representation\n";
		case TRIO_TREE_ERR:
			return "Tree error happened while reading the tree\n";
		case TRIO_NO_ERR:
			return "No error occured\n";
		default:
			return "Unknown error occured\n";
	}
}
