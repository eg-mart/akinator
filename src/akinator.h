#include "tree.h"
#include "buffer.h"

enum AkError {
	AK_NO_ERR = 0,
	AK_FOUND = 1,
};

const int ANS_BUF_SIZE =  512;

enum AkError describe(const struct Node *tr);
enum AkError compare(const struct Node *tr);
enum AkError guess(struct Node **tr, struct Buffer *buf);