#include "tree.h"
#include "buffer.h"

enum AkErrorCode {
	AK_ELEM_NOT_FOUND_ERR = -5,
	AK_ANS_READ_ERR = -4,
	AK_STACK_ERR = -3,
	AK_TREE_ERR = -2,
	AK_BUFFER_ERR = -1,
	AK_NO_ERR = 0,
};

const size_t OUTPUT_BUF_SIZE	= 1024;
const size_t ANSWER_BUF_SIZE    = 512;
const size_t ERR_CONTEXT_SIZE 	= 128;

struct AkError {
	enum AkErrorCode code;

	char context[ERR_CONTEXT_SIZE];
};


struct AkError describe(const struct Node *tr, bool do_speak);
struct AkError compare(const struct Node *tr, bool do_speak);
struct AkError guess(struct Node **tr, struct Buffer *buf, bool do_speak);
struct AkError compose_err(enum AkErrorCode code, const char *context);
void ak_err_to_str(char *str, struct AkError err, size_t n);
