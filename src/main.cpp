#include <stdlib.h>
#include <string.h>

#include "tree.h"
#include "logger.h"

void guess(struct Tree *tr);

int main()
{
	logger_ctor();
	add_log_handler({stderr, DEBUG, true});
	FILE *inpt = fopen("tree.txt", "r");
	if (!inpt) {
		log_message(ERROR, "Error opening input file\n");
		return 1;
	}
	struct Tree tr = {};
	tree_ctor(&tr);
	tree_load(&tr, inpt);
	printf("------------\n");
	guess(&tr);
	tree_dump(&tr);
	tree_dtor(&tr);
	fclose(inpt);
	logger_dtor();
	return 0;
}

void guess(struct Tree *tr)
{
	struct Node *cur_node = tr->root;
	while (true) {
		char ans[1024] = {};
		printf("%s?\n", cur_node->data);
		scanf("%32s", ans);
		struct Node *new_node = cur_node;
		if (strcmp(ans, "да") == 0)
			new_node = cur_node->left;
		else if (strcmp(ans, "нет") == 0)
			new_node = cur_node->right;
		else
			printf("Wrong answer! Try again.\n");
		if (!new_node) {
			printf("Хз кто это. Кто это?\n");
			scanf("%s", ans);
			node_op_new(&cur_node->left, ans);
			node_op_new(&cur_node->right, cur_node->data);
			printf("Как он отличается от %s?\n", cur_node->data);
			scanf("%s", ans);
			free(cur_node->data);
			cur_node->data = strdup(ans);
			return;
		} else {
			cur_node = new_node;
		}
	}
}