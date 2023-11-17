#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>

#include "tree.h"
#include "logger.h"

static void _node_dump(struct Node *node, size_t level);
static void read_word(char *word, size_t size, FILE *input);

void tree_ctor(struct Tree *tr)
{
	tr->size = 0;
	tr->root = NULL;
}

enum TreeError node_op_new(struct Node **node, const char *data)
{
	assert(node);
	assert(data);

	*node = (struct Node*) calloc(1, sizeof(struct Node));
	if (!(*node))
		return TREE_NO_MEM_ERR;

	return node_ctor(*node, data);
}

enum TreeError node_ctor(struct Node *node, const char *data)
{
	assert(node);
	assert(data);

	node->data = strdup(data);
	if (!node->data)
		return TREE_NO_MEM_ERR;
	node->left = NULL;
	node->right = NULL;

	return TREE_NO_ERR;
}

void tree_dump(struct Tree *tree)
{
	log_message(DEBUG, "-------------------\n");
	log_message(DEBUG, "Dumping tree of size %d:\n");
	node_dump(tree->root);
}

void node_dump(struct Node *node)
{
	_node_dump(node, 0);
}

static void _node_dump(struct Node *node, size_t level)
{
	log_message(DEBUG, "   ");
	for (size_t i = 0; i < level; i++)
		log_string(DEBUG, "   ");
	if (!node) {
		log_string(DEBUG, "    nil\n");
		return;
	}
	log_string(DEBUG, "{\n");

	log_message(DEBUG, "   ");
	for (size_t i = 0; i < level; i++)
		log_string(DEBUG, "   ");
	log_string(DEBUG, "   %s\n", node->data);

	_node_dump(node->left, level + 1);
	_node_dump(node->right, level + 1);
	
	log_message(DEBUG, "   ");
	for (size_t i = 0; i < level; i++)
		log_string(DEBUG, "   ");
	log_string(DEBUG, "}\n");
}

void tree_dtor(struct Tree *tr)
{
	tr->size = 0;
	node_op_delete(tr->root);
}

enum TreeError node_load(struct Node **node, FILE *input)
{
	const size_t WORD_SIZE = 128;
	char word[WORD_SIZE] = "";

	int symb = getc(input);
	switch (symb) {
		case '(':
			read_word(word, WORD_SIZE, input);
			node_op_new(node, word);
			node_load(&(*node)->left, input);
			node_load(&(*node)->right, input);
			return TREE_NO_ERR;
		default:
			read_word(word, WORD_SIZE, input);
			log_message(INFO, "read word %s\n", word);
			*node = NULL;
			return TREE_NO_ERR;
	}
}

static void read_word(char *word, size_t size, FILE *input)
{
	int symb = getc(input);
	size_t ind = 0;
	while (ind < size && symb != EOF && !isspace(symb)) {
		word[ind] = (char) symb;
		ind++;
		symb = getc(input);
	}
}

enum TreeError tree_load(struct Tree *tr, FILE *input)
{
	return node_load(&tr->root, input);
}

void node_op_delete(struct Node *node)
{
	if (!node)
		return;
	
	free(node->data);
	node_op_delete(node->left);
	node_op_delete(node->right);
	free(node);
}

const char *tree_err_to_str(enum TreeError err)
{
	switch (err) {
		case TREE_NO_MEM_ERR:
			return "No memory\n";
		case TREE_NO_ERR:
			return "No error occured\n";
		default:
			return "An unknown error occured\n";
	}
}