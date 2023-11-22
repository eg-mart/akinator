#ifndef _CMD_ARGS_H
#define _CMD_ARGS_H

const size_t LONG_NAME_SIZE = 128;
const size_t DESCRIPTION_SIZE = 1024;

enum ArgError {
	ARG_WRONG_ARGS_ERR			= -5,
	ARG_WRONG_POS_ERR			= -4,
	ARG_MISSING_ERR				= -3,
	ARG_MISSING_REQUIRED_ERR	= -2,
	ARG_NO_ERR					= 0,
};

typedef enum ArgError (*arg_handler)(const char *arg_str, void *processed_args);

struct ArgDef {
	char long_name[LONG_NAME_SIZE];
	char short_name;
	char description[DESCRIPTION_SIZE];
	bool is_optional;
	bool is_flag;
	arg_handler handler;
};

enum ArgError process_args(const struct ArgDef arg_defs[], size_t arg_defs_size,
						   const char *argv[], int argc, void *processed_args);
const char *arg_err_to_str(enum ArgError err);

#endif /*_CMD_ARGS_H*/
