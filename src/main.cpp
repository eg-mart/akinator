#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "tree.h"
#include "tree_debug.h"
#include "logger.h"
#include "tree_io.h"
#include "buffer.h"
#include "cmd_args.h"

struct CmdArgs {
	const char *input_filename;
	const char *output_filename;
	const char *dump_filename;
	bool guess_mode;
};

enum ArgError handle_input_filename(const char *arg_str, void *processed_args);
enum ArgError handle_output_filename(const char *arg_str, void *processed_args);
enum ArgError handle_dump_filename(const char *arg_str, void *processed_args);
enum ArgError handle_guess_mode(const char *arg_str, void *processed_args);

void guess(struct Node **tr, struct Buffer *buf);
void print_str(char *buf, size_t n, const char *data);

const struct ArgDef arg_defs[] = {
	{"input", 'i', "Name of input database's file", 
	 false, false, handle_input_filename},

	{"output", 'o', "Name of output database's file",
	 false, false, handle_output_filename},

	{"dump", 'd', "Name of the html dump file",
	 false, false, handle_dump_filename},

	{"guess", 'g', "Enable guessing mode",
	 true, true, handle_guess_mode},
};

int main(int argc, const char *argv[])
{
	logger_ctor();
	add_log_handler({stderr, DEBUG, true});

	struct CmdArgs args = { NULL, NULL, NULL, false };
	enum ArgError arg_err = ARG_NO_ERR;
	arg_err = process_args(arg_defs, sizeof(arg_defs) / sizeof(arg_defs[0]),
					 	   argv, argc, &args);
	if (arg_err < 0) {
		log_message(ERROR, arg_err_to_str(arg_err));
		logger_dtor();
		return arg_err;
	}
	if (arg_err == ARG_HELP_CALLED) {
		logger_dtor();
		return 0;
	}
	
	struct Buffer buf = {};
	buffer_ctor(&buf, args.input_filename);
	struct Buffer ans_buf = {};
	ans_buf.data = (char*) calloc(2048, sizeof(char));
	ans_buf.size = 2048;

	struct Node *tr = NULL;
	enum TreeIOError err = tree_load_from_buf(&tr, &buf);
	if (err < 0)
		printf("%d\n", err);

	FILE *dump_html = tree_start_html_dump(args.dump_filename);
	TREE_DUMP_GUI(tr, dump_html, print_str);

	guess(&tr, &ans_buf);

	TREE_DUMP_GUI(tr, dump_html, print_str);
	FILE *save_file = fopen(args.output_filename, "w");
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

void print_str(char *buf, size_t n, const char *data)
{
	snprintf(buf, n, "%s", data);
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

enum ArgError handle_input_filename(const char *arg_str, void *processed_args)
{
	struct CmdArgs *args = (struct CmdArgs*) processed_args;
	args->input_filename = arg_str;
	return ARG_NO_ERR;
}

enum ArgError handle_output_filename(const char *arg_str, void *processed_args)
{
	struct CmdArgs *args = (struct CmdArgs*) processed_args;
	args->output_filename = arg_str;
	return ARG_NO_ERR;
}

enum ArgError handle_dump_filename(const char *arg_str, void *processed_args)
{
	struct CmdArgs *args = (struct CmdArgs*) processed_args;
	args->dump_filename = arg_str;
	return ARG_NO_ERR;
}

enum ArgError handle_guess_mode(const char *arg_str, void *processed_args)
{
	struct CmdArgs *args = (struct CmdArgs*) processed_args;
	args->guess_mode = true;
	return ARG_NO_ERR;
}
