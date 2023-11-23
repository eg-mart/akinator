#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "tree.h"
#include "tree_debug.h"
#include "logger.h"
#include "tree_io.h"
#include "buffer.h"
#include "cmd_args.h"
#include "stack.h"
#include "stack_debug.h"

struct CmdArgs {
	const char *input_filename;
	const char *output_filename;
	const char *dump_filename;
	bool guess_mode;
	bool comparison_mode;
	bool description_mode;
};

enum ArgError handle_input_filename(const char *arg_str, void *processed_args);
enum ArgError handle_output_filename(const char *arg_str, void *processed_args);
enum ArgError handle_dump_filename(const char *arg_str, void *processed_args);
enum ArgError handle_guess_mode(const char *arg_str, void *processed_args);
enum ArgError handle_comparison_mode(const char *arg_str, void *processed_args);
enum ArgError handle_description_mode(const char *arg_str, void *processed_args);

int describe(const struct Node *tr);
int compare(const struct Node *tr);
void guess(struct Node **tr, struct Buffer *buf);
int make_stack(const struct Node *tr, const char *elem, struct Stack *stk);
void print_str(char *buf, const char *data, size_t n);
void print_int(char *buf, int data, size_t n);
void cut_after_newline(char *str, size_t n);

const int ANS_BUF_SIZE = 512;

const struct ArgDef arg_defs[] = {
	{"input", 'i', "Name of the input database's file", 
	 false, false, handle_input_filename},

	{"output", 'o', "Name of the output database's file",
	 false, false, handle_output_filename},

	{"dump", 'm', "Name of the html dump file",
	 false, false, handle_dump_filename},

	{"guess", 'g', "Enable guessing mode",
	 true, true, handle_guess_mode},

	{"compare", 'c', "Enable comparison mode",
	 true, true, handle_comparison_mode},

	{"describe", 'd', "Enable description mode",
	 true, true, handle_description_mode},
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
	buffer_ctor(&buf);
	buffer_load_from_file(&buf, args.input_filename);
	struct Buffer ans_buf = {};
	buffer_ctor(&ans_buf);

	struct Node *tr = NULL;
	enum TreeIOError err = tree_load_from_buf(&tr, &buf);
	if (err < 0)
		printf("%d\n", err);

	FILE *dump_html = tree_start_html_dump(args.dump_filename);
	TREE_DUMP_GUI(tr, dump_html, print_str);

	if (args.guess_mode)
		guess(&tr, &ans_buf);
	else if (args.description_mode)
		describe(tr);
	else if (args.comparison_mode)
		compare(tr);

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

void print_str(char *buf, const char *data, size_t n)
{
	snprintf(buf, n, "%s", data);
}

void print_int(char *buf, int data, size_t n)
{
	snprintf(buf, n, "%d", data);
}

void guess(struct Node **tr, struct Buffer *buf)
{
	struct Node *cur_node = *tr;
	
	while (true) {
		char ans[ANS_BUF_SIZE] = {};
		if (!cur_node->left && !cur_node->right)
			printf("Это же %s! Да?\n", cur_node->data);
		else
			printf("Оно %s?\n", cur_node->data);

		fgets(ans, ANS_BUF_SIZE, stdin);
		struct Node *new_node = cur_node;
		if (strcmp(ans, "да\n") == 0) {
			new_node = cur_node->left;
			if (!new_node) {
				printf("Ура я угадал!\n");
				return;
			}
		}
		else if (strcmp(ans, "нет\n") == 0) {
			new_node = cur_node->right;
		} else {
			printf("Неправильный ответ! Попробуйте снова.\n");
		}
		
		if (!new_node) {
			printf("Хз кто это. Кто это?\n");

			log_message(INFO, "%p\n", buf->pos);
			fgets(buf->pos, ANS_BUF_SIZE, stdin);
			cut_after_newline(buf->pos, ANS_BUF_SIZE);

			node_op_new(&cur_node->left, buf->pos);
			node_op_new(&cur_node->right, cur_node->data);

			buf->pos += strlen(buf->pos) + 1;

			printf("Как он отличается от %s?\n", cur_node->data);
			fgets(buf->pos, ANS_BUF_SIZE, stdin);
			cut_after_newline(buf->pos, ANS_BUF_SIZE);

			cur_node->data = buf->pos;
			buf->pos += strlen(buf->pos) + 1;
			return;
		} else {
			cur_node = new_node;
		}
	}
}

int make_stack(const struct Node *tr, const char *elem, struct Stack *stk)
{
	assert(elem);
	assert(stk);

	if (!tr)
		return 0;

	if (strcmp(tr->data, elem) == 0)
		return 1;

	if (make_stack(tr->left, elem, stk)) {
		stack_push(stk, 0);
		return 1;
	}
	if (make_stack(tr->right, elem, stk)) {
		stack_push(stk, 1);
		return 1;
	}
	
	return 0;
}

int describe(const struct Node *tr)
{
	char ans_buf[ANS_BUF_SIZE] = {};
	puts("Кого хочешь описать?");
	fgets(ans_buf, ANS_BUF_SIZE, stdin);
	cut_after_newline(ans_buf, ANS_BUF_SIZE);
	printf("Окей! %s:\n", ans_buf);
	
	struct Stack stk = {};
	STACK_CTOR(&stk, print_int);
	
	make_stack(tr, ans_buf, &stk);

	const struct Node *cur_node = tr;
	int val = 0;
	enum StackError err = stack_pop(&stk, &val);
	while (err >= 0) {
		if (val) {
			printf("-Не %s\n", cur_node->data);
			cur_node = cur_node->right;
		} else {
			printf("-%s\n", cur_node->data);
			cur_node = cur_node->left;
		}
		err = stack_pop(&stk, &val);
	}
	stack_dtor(&stk);

	return 0;
}

int compare(const struct Node *tr)
{
	char ans1_buf[ANS_BUF_SIZE] = {};
	char ans2_buf[ANS_BUF_SIZE] = {};

	puts("Кого хочешь сравнить?");
	fgets(ans1_buf, ANS_BUF_SIZE, stdin);
	cut_after_newline(ans1_buf, ANS_BUF_SIZE);
	puts("И с кем?");
	fgets(ans2_buf, ANS_BUF_SIZE, stdin);
	cut_after_newline(ans2_buf, ANS_BUF_SIZE);

	struct Stack stk1 = {};
	struct Stack stk2 = {};
	STACK_CTOR(&stk1, print_int);
	STACK_CTOR(&stk2, print_int);

	make_stack(tr, ans1_buf, &stk1);
	make_stack(tr, ans2_buf, &stk2);

	printf("И %s, и %s:\n", ans1_buf, ans2_buf);
	const struct Node *cur_node = tr;
	int val1 = 0;
	int val2 = 0;
	enum StackError err = STACK_NO_ERR;
	while (true) {
		err = stack_pop(&stk1, &val1);
		if (err < 0)
			break;
		err = stack_pop(&stk2, &val2);
		if (err < 0)
			break;
		if (val1 != val2)
			break;
		if (val1) {
			printf("-Не %s\n", cur_node->data);
			cur_node = cur_node->right;
		} else {
			printf("-%s\n", cur_node->data);
			cur_node = cur_node->left;
		}
	}

	err = STACK_NO_ERR;
	printf("Помимо этого, %s:\n", ans1_buf);
	const struct Node *cur_node1 = cur_node;
	while (err >= 0) {
		if (val1) {
			printf("-Не %s\n", cur_node1->data);
			cur_node1 = cur_node1->right;
		} else {
			printf("-%s\n", cur_node1->data);
			cur_node1 = cur_node1->left;
		}
		err = stack_pop(&stk1, &val1);
	}

	err = STACK_NO_ERR;
	printf("Помимо этого, %s:\n", ans2_buf);
	const struct Node *cur_node2 = cur_node;
	while (err >= 0) {
		if (val2) {
			printf("-Не %s\n", cur_node2->data);
			cur_node2 = cur_node2->right;
		} else {
			printf("-%s\n", cur_node2->data);
			cur_node2 = cur_node2->left;
		}
		err = stack_pop(&stk2, &val2);
	}

	stack_dtor(&stk1);
	stack_dtor(&stk2);
	return 0;
}

void cut_after_newline(char *str, size_t n)
{
	for (size_t i = 0; i < n; i++) {
		if (str[i] == '\n') {
			str[i] = '\0';
			break;
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

enum ArgError handle_guess_mode(const char */*arg_str*/, void *processed_args)
{
	struct CmdArgs *args = (struct CmdArgs*) processed_args;
	args->guess_mode = true;
	return ARG_NO_ERR;
}

enum ArgError handle_comparison_mode(const char */*arg_str*/, void *processed_args)
{
	struct CmdArgs *args = (struct CmdArgs*) processed_args;
	args->comparison_mode = true;
	return ARG_NO_ERR;
}

enum ArgError handle_description_mode(const char */*arg_str*/, void *processed_args)
{
	struct CmdArgs *args = (struct CmdArgs*) processed_args;
	args->description_mode = true;
	return ARG_NO_ERR;
}