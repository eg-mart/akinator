#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "tree.h"
#include "tree_debug.h"
#include "logger.h"
#include "tree_io.h"
#include "buffer.h"

void guess(struct Node **tr, struct Buffer *buf);

void print_str(char *buf, size_t n, const char *data)
{
	snprintf(buf, n, "%s", data);
}

int main()
{
	logger_ctor();
	add_log_handler({stderr, DEBUG, true});
	
	struct Buffer buf = {};
	buffer_ctor(&buf, "save.txt");
	struct Buffer ans_buf = {};
	ans_buf.data = (char*) calloc(2048, sizeof(char));
	ans_buf.size = 2048;

	struct Node *tr = NULL;
	enum TreeIOError err = tree_load_from_buf(&tr, &buf);
	if (err < 0)
		printf("%d\n", err);

	FILE *dump_html = tree_start_html_dump("dump.html");
	TREE_DUMP_GUI(tr, dump_html, print_str);

	guess(&tr, &ans_buf);

	TREE_DUMP_GUI(tr, dump_html, print_str);
	FILE *save_file = fopen("save.txt", "w");
	if (!save_file)
		log_message(ERROR, "Error opening save file\n");
	tree_save(tr, save_file);

	fclose(save_file);
	node_op_delete(tr);
	tree_end_html_dump(dump_html);
	buffer_dtor(&buf);
	buffer_dtor(&ans_buf);
	logger_dtor();
	
	return 0;
}

void guess(struct Node **tr, struct Buffer *buf)
{
	struct Node *cur_node = *tr;
	char *buf_iter = buf->data;
	
	while (true) {
		char ans[1024] = {};
		if (!cur_node->left && !cur_node->right)
			printf("Это же %s! Да?\n", cur_node->data);
		else
			printf("Оно %s?\n", cur_node->data);
		scanf("%32s", ans);
		struct Node *new_node = cur_node;
		if (strcmp(ans, "да") == 0) {
			new_node = cur_node->left;
			if (!new_node) {
				printf("Ура я угадал!\n");
				return;
			}
		}
		else if (strcmp(ans, "нет") == 0)
			new_node = cur_node->right;
		else
			printf("Неправильный ответ! Попробуйте снова.\n");
		
		if (!new_node) {
			printf("Хз кто это. Кто это?\n");

			int read = 0;
			while (read != '\n' && read != EOF)
				read = getchar();

			fgets(buf_iter, 32, stdin);
			for (size_t i = 0; buf_iter[i] != '\0'; i++) {
				if (buf_iter[i] == '\n') {
					buf_iter[i] = '\0';
					break;
				}
			}

			node_op_new(&cur_node->left, buf_iter);
			node_op_new(&cur_node->right, cur_node->data);

			buf_iter += strlen(buf_iter) + 1;

			printf("Как он отличается от %s?\n", cur_node->data);
			fgets(buf_iter, 32, stdin);
			for (size_t i = 0; buf_iter[i] != '\0'; i++) {
				if (buf_iter[i] == '\n') {
					buf_iter[i] = '\0';
					break;
				}
			}

			cur_node->data = buf_iter;
			buf_iter += strlen(buf_iter) + 1;
			return;
		} else {
			cur_node = new_node;
		}
	}
}