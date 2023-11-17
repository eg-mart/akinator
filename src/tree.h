#include <stdio.h>

struct Node {
	char *data;
	struct Node *left;
	struct Node *right;
};

struct Tree {
	struct Node *root;
	size_t size;
};

enum TreeError {
	TREE_NO_MEM_ERR = -1,
	TREE_NO_ERR 	= 0,
};

void tree_ctor(struct Tree *tr);
enum TreeError node_op_new(struct Node **node, const char *data);
enum TreeError node_ctor(struct Node *node, const char *data);
enum TreeError tree_load(struct Tree *tr, FILE *input);
enum TreeError node_load(struct Node **node, FILE *input);
void tree_dump(struct Tree *tr);
void node_dump(struct Node *node);
void tree_dtor(struct Tree *tr);
void node_op_delete(struct Node *node);
const char *tree_err_to_str(enum TreeError err);
