#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>

#include "stack.h"
#include "stack_debug.h"
#include "akinator.h"
#include "logger.h"

static struct AkError make_definition_stack(const struct Node *tr,
											const char *elem, struct Stack *stk);
static void cut_after_newline(char *str, size_t n);
static void print_int(char *buf, int data, size_t n);
static void ak_output(bool do_speek, const char *fmt, ...);
static char *skip_space(char *str);

struct AkError guess(struct Node **tr, struct Buffer *buf, bool do_speak)
{
	struct Node *cur_node = *tr;

	while (true) {
		char ans[ANSWER_BUF_SIZE] = {};
		if (!cur_node->left && !cur_node->right)
			ak_output(do_speak, "Это же %s! Да?\n", cur_node->data);
		else
			ak_output(do_speak, "Оно %s?\n", cur_node->data);

		char *read = fgets(ans, ANSWER_BUF_SIZE, stdin);
		if (!read)
			return compose_err(AK_ANS_READ_ERR, "");

		struct Node *new_node = cur_node;
		if (strcmp(ans, "да\n") == 0) {
			new_node = cur_node->left;
			if (!new_node) {
				ak_output(do_speak, "Ура я угадал!\n");
				return compose_err(AK_NO_ERR, "");
			}
		}
		else if (strcmp(ans, "нет\n") == 0) {
			new_node = cur_node->right;
		} else {
			ak_output(do_speak, "Неправильный ответ! Попробуйте снова.\n");
		}

		if (!new_node) {
			ak_output(do_speak, "Хз кто это. Кто это?\n");

			if (buffer_size(buf) >= buf->cap)
				return compose_err(AK_BUFFER_ERR, "");
			read = fgets(buf->pos, (int) (buf->cap - buffer_size(buf)), stdin);
			if (!read)
				return compose_err(AK_ANS_READ_ERR, "");
			buf->pos = skip_space(buf->pos);
			cut_after_newline(buf->pos, ANSWER_BUF_SIZE);

			enum TreeError tr_err = node_op_new(&cur_node->left, buf->pos);
			if (tr_err < 0)
				return compose_err(AK_TREE_ERR, tree_err_to_str(tr_err));
			tr_err = node_op_new(&cur_node->right, cur_node->data);
			if (tr_err < 0)
				return compose_err(AK_TREE_ERR, tree_err_to_str(tr_err));

			buf->pos += strlen(buf->pos) + 1;

			if (buffer_size(buf) >= buf->cap)
				return compose_err(AK_BUFFER_ERR, "");
			ak_output(do_speak, "Как он отличается от %s?\n", cur_node->data);
			read = fgets(buf->pos, (int) (buf->cap - buffer_size(buf)), stdin);
			if (!read)
				return compose_err(AK_ANS_READ_ERR, "");
			buf->pos = skip_space(buf->pos);
			cut_after_newline(buf->pos, ANSWER_BUF_SIZE);

			cur_node->data = buf->pos;
			buf->pos += strlen(buf->pos) + 1;
			return compose_err(AK_NO_ERR, "");
		} else {
			cur_node = new_node;
		}
	}
	return compose_err(AK_NO_ERR, "");
}

// descr def
static struct AkError make_definition_stack(const struct Node *tr, 
											const char *elem, struct Stack *stk)
{
	assert(elem);
	assert(stk);

	enum StackError stk_err = STACK_NO_ERR;
	struct AkError err = compose_err(AK_NO_ERR, "");
	if (!tr)
		return compose_err(AK_ELEM_NOT_FOUND_ERR, elem);

	if (strcmp(tr->data, elem) == 0)
		return err;

	err = make_definition_stack(tr->left, elem, stk);
	if (err.code == AK_NO_ERR) {
		stk_err = stack_push(stk, 0);
		if (stk_err < 0)
			return compose_err(AK_STACK_ERR, stack_err_to_str(stk_err));
		return err;
	}
	if (err.code != AK_ELEM_NOT_FOUND_ERR)
		return err;

	err = make_definition_stack(tr->right, elem, stk);
	if (err.code == AK_NO_ERR) {
		stk_err = stack_push(stk, 1);
		if (stk_err < 0)
			return compose_err(AK_STACK_ERR, stack_err_to_str(stk_err));
		return compose_err(AK_NO_ERR, "");
	}

	return err;
}

struct AkError describe(const struct Node *tr, bool do_speak)
{
	char ans_buf[ANSWER_BUF_SIZE] = {};
	ak_output(do_speak, "Кого хочешь описать?\n");
	char *read = fgets(ans_buf, ANSWER_BUF_SIZE, stdin);
	if (!read)
		return compose_err(AK_ANS_READ_ERR, "");
	cut_after_newline(ans_buf, ANSWER_BUF_SIZE);
	ak_output(do_speak, "Окей! %s:\n", ans_buf);

	struct Stack stk = {};
	enum StackError stk_err = STACK_CTOR(&stk, print_int);
	if (stk_err < 0) {
		stack_dtor(&stk);
		return compose_err(AK_STACK_ERR, stack_err_to_str(stk_err));
	}

	struct AkError err = make_definition_stack(tr, ans_buf, &stk);
	if (err.code < 0) {
		stack_dtor(&stk);
		return err;
	}

	const struct Node *cur_node = tr;
	int val = 0;
	stk_err = stack_pop(&stk, &val);
	if (stk_err < 0) {
		stack_dtor(&stk);
		return compose_err(AK_STACK_ERR, stack_err_to_str(stk_err));
	}
	while (stk_err >= 0) {
		if (val) {
			ak_output(do_speak, "-Не %s\n", cur_node->data);
			cur_node = cur_node->right;
		} else {
			ak_output(do_speak, "-%s\n", cur_node->data);
			cur_node = cur_node->left;
		}
		stk_err = stack_pop(&stk, &val);
	}
	if (stk_err != ERR_STACK_EMPTY && stk_err < 0) {
		stack_dtor(&stk);
		return compose_err(AK_STACK_ERR, stack_err_to_str(stk_err));
	}
	stack_dtor(&stk);

	return compose_err(AK_NO_ERR, "");
}

struct AkError compare(const struct Node *tr, bool do_speak)
{
	char ans1_buf[ANSWER_BUF_SIZE] = {};
	char ans2_buf[ANSWER_BUF_SIZE] = {};

	ak_output(do_speak, "Кого хочешь сравнить?\n");
	char *read = fgets(ans1_buf, ANSWER_BUF_SIZE, stdin);
	if (!read)
		return compose_err(AK_NO_ERR, "");
	cut_after_newline(ans1_buf, ANSWER_BUF_SIZE);

	ak_output(do_speak, "И с кем?\n");
	read = fgets(ans2_buf, ANSWER_BUF_SIZE, stdin);
	if (!read)
		return compose_err(AK_NO_ERR, "");
	cut_after_newline(ans2_buf, ANSWER_BUF_SIZE);

	struct Stack stk1 = {};
	struct Stack stk2 = {};
	enum StackError stk_err = STACK_CTOR(&stk1, print_int);
	if (stk_err < 0) {
		stack_dtor(&stk1);
		return compose_err(AK_STACK_ERR, stack_err_to_str(stk_err));
	}
	stk_err = STACK_CTOR(&stk2, print_int);
	if (stk_err < 0) {
		stack_dtor(&stk1);
		stack_dtor(&stk2);
		return compose_err(AK_STACK_ERR, stack_err_to_str(stk_err));
	}

	struct AkError err = make_definition_stack(tr, ans1_buf, &stk1);
	if (err.code < 0) {
		stack_dtor(&stk1);
		stack_dtor(&stk2);
		return err;
	}
	err = make_definition_stack(tr, ans2_buf, &stk2);
	if (err.code < 0) {
		stack_dtor(&stk1);
		stack_dtor(&stk2);
		return err;
	}

	ak_output(do_speak, "И %s, и %s:\n", ans1_buf, ans2_buf);
	const struct Node *cur_node = tr;
	int val1 = 0;
	int val2 = 0;
	enum StackError stk_err1 = STACK_NO_ERR;
	enum StackError stk_err2 = STACK_NO_ERR;
	while (true) {
		stk_err1 = stack_pop(&stk1, &val1);
		if (stk_err1 < 0)
			break;
		stk_err2 = stack_pop(&stk2, &val2);
		if (stk_err2 < 0)
			break;
		if (val1 != val2)
			break;
		if (val1) {
			ak_output(do_speak, "-Не %s\n", cur_node->data);
			cur_node = cur_node->right;
		} else {
			ak_output(do_speak, "-%s\n", cur_node->data);
			cur_node = cur_node->left;
		}
	}

	if (stk_err1 < 0 && stk_err1 != ERR_STACK_EMPTY) {
		stack_dtor(&stk1);
		stack_dtor(&stk2);
		return compose_err(AK_STACK_ERR, stack_err_to_str(stk_err1));
	}
	if (stk_err2 < 0 && stk_err2 != ERR_STACK_EMPTY) {
		stack_dtor(&stk1);
		stack_dtor(&stk2);
		return compose_err(AK_STACK_ERR, stack_err_to_str(stk_err2));
	}

	ak_output(do_speak, "Помимо этого, %s:\n", ans1_buf);
	const struct Node *cur_node1 = cur_node; // func
	while (stk_err1 >= 0) {
		if (val1) {
			ak_output(do_speak, "-Не %s\n", cur_node1->data);
			cur_node1 = cur_node1->right;
		} else {
			ak_output(do_speak, "-%s\n", cur_node1->data);
			cur_node1 = cur_node1->left;
		}
		stk_err1 = stack_pop(&stk1, &val1);
	}

	if (stk_err1 < 0 && stk_err1 != ERR_STACK_EMPTY) {
		stack_dtor(&stk1);
		stack_dtor(&stk2);
		return compose_err(AK_STACK_ERR, stack_err_to_str(stk_err1));
	}

	ak_output(do_speak,  "Помимо этого, %s:\n", ans2_buf);
	const struct Node *cur_node2 = cur_node;
	while (stk_err2 >= 0) {
		if (val2) {
			ak_output(do_speak, "-Не %s\n", cur_node2->data);
			cur_node2 = cur_node2->right;
		} else {
			ak_output(do_speak, "-%s\n", cur_node2->data);
			cur_node2 = cur_node2->left;
		}
		stk_err2 = stack_pop(&stk2, &val2);
	}

	if (stk_err2 < 0 && stk_err2 != ERR_STACK_EMPTY) {
		stack_dtor(&stk1);
		stack_dtor(&stk2);
		return compose_err(AK_STACK_ERR, stack_err_to_str(stk_err1));
	}

	stack_dtor(&stk1);
	stack_dtor(&stk2);
	return compose_err(AK_NO_ERR, "");
}

static void cut_after_newline(char *str, size_t n)
{
	for (size_t i = 0; i < n; i++) {
		if (str[i] == '\n') {
			str[i] = '\0';
			break;
		}
	}
}

static void print_int(char *buf, int data, size_t n)
{
	snprintf(buf, n, "%d", data);
}

struct AkError compose_err(enum AkErrorCode code, const char *context)
{
	struct AkError err = {code, ""};
	strncpy(err.context, context, ERR_CONTEXT_SIZE);
	return err;
}

void ak_err_to_str(char *str, struct AkError err, size_t n)
{
	switch (err.code) {
		case AK_ELEM_NOT_FOUND_ERR:
			strncpy(str, "Element wasn't found in the tree: ", n);
			strncat(str, err.context, n - strlen(str));
			return;
		case AK_ANS_READ_ERR:
			strncpy(str, "Error reading the answer", n);
			return;
		case AK_STACK_ERR:
			strncpy(str, "Error in the stack: ", n);
			strncat(str, err.context, n - strlen(str));
			return;
		case AK_TREE_ERR:
			strncpy(str, "Error in the tree: ", n);
			strncat(str, err.context, n - strlen(str));
			return;
		case AK_BUFFER_ERR:
			strncpy(str, "Buffer overflow while trying to save answers", n);
			return;
		case AK_NO_ERR:
			strncpy(str, "No error occured", n);
			return;
		default:
			strncpy(str, "Unknown error occured", n);
			return;
	}
}

static void ak_output(bool do_speek, const char *fmt, ...)
{
	assert(fmt);

	va_list args;
	va_start(args, fmt);

	char buf[OUTPUT_BUF_SIZE] = "";
	vsnprintf(buf, OUTPUT_BUF_SIZE, fmt, args);
	fputs(buf, stdout);

	if (do_speek) {
		char cmd[2 * OUTPUT_BUF_SIZE] = "echo \"";
		strncat(cmd, buf, 2 * OUTPUT_BUF_SIZE - strlen(cmd));
		strncat(cmd, "\" | festival --tts", 2 * OUTPUT_BUF_SIZE - strlen(cmd));
		system(cmd);
	}
}

static char *skip_space(char *str)
{
	assert(str);

	while (isspace(*str))
		str++;
	return str;
}

