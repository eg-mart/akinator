#include "tree.h"
#include "buffer.h"

enum AkErrorCode {
	AK_STACK_ERR = -3,
	AK_TREE_ERR = -2,
	AK_BUFFER_ERR = -1,
	AK_NO_ERR = 0,
	AK_FOUND = 1,
};

struct AkError {
	enum AkErrorCode code;
	char *context;
};

const int ANS_BUF_SIZE =  512;

enum AkErrorCode describe(const struct Node *tr);
enum AkErrorCode compare(const struct Node *tr);
enum AkErrorCode guess(struct Node **tr, struct Buffer *buf);