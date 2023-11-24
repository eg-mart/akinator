#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "stack.h"
#include "akinator.h"
#include "logger.h"

static enum AkError make_stack(const struct Node *tr, const char *elem,
							   struct Stack *stk);
static void cut_after_newline(char *str, size_t n);
static void print_int(char *buf, int data, size_t n);

enum AkError guess(struct Node **tr, struct Buffer *buf)
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
				return AK_NO_ERR;
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
			return AK_NO_ERR;
		} else {
			cur_node = new_node;
		}
	}
	return AK_NO_ERR;
}

static enum AkError make_stack(const struct Node *tr, const char *elem,
							   struct Stack *stk)
{
	assert(elem);
	assert(stk);

	if (!tr)
		return AK_NO_ERR;

	if (strcmp(tr->data, elem) == 0)
		return AK_FOUND;

	if (make_stack(tr->left, elem, stk)) {
		stack_push(stk, 0);
		return AK_FOUND;
	}
	if (make_stack(tr->right, elem, stk)) {
		stack_push(stk, 1);
		return AK_FOUND;
	}
	
	return AK_NO_ERR;;
}

enum AkError describe(const struct Node *tr)
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

	return AK_NO_ERR;
}

enum AkError compare(const struct Node *tr)
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

	enum StackError err1 = err;
	printf("Помимо этого, %s:\n", ans1_buf);
	const struct Node *cur_node1 = cur_node;
	while (err1 >= 0) {
		if (val1) {
			printf("-Не %s\n", cur_node1->data);
			cur_node1 = cur_node1->right;
		} else {
			printf("-%s\n", cur_node1->data);
			cur_node1 = cur_node1->left;
		}
		err1 = stack_pop(&stk1, &val1);
	}

	enum StackError err2 = err;
	printf("Помимо этого, %s:\n", ans2_buf);
	const struct Node *cur_node2 = cur_node;
	while (err2 >= 0) {
		if (val2) {
			printf("-Не %s\n", cur_node2->data);
			cur_node2 = cur_node2->right;
		} else {
			printf("-%s\n", cur_node2->data);
			cur_node2 = cur_node2->left;
		}
		err2 = stack_pop(&stk2, &val2);
	}

	stack_dtor(&stk1);
	stack_dtor(&stk2);
	return AK_NO_ERR;
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


