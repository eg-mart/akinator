#ifndef _TREE_H
#define _TREE_H

typedef const char* elem_t;

struct Node {
	elem_t data;
	struct Node *left;
	struct Node *right;
};

enum TreeError {
	TREE_NO_MEM_ERR = -1,
	TREE_NO_ERR 	= 0,
};

enum TreeError node_op_new(struct Node **node, elem_t data);
void node_ctor(struct Node *node, elem_t data);
void node_op_delete(struct Node *node);
const char *tree_err_to_str(enum TreeError err);

#endif /*_TREE_H*/