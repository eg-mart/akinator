#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "tree.h"
#include "tree_debug.h"
#include "logger.h"
#include "tree_io.h"
#include "buffer.h"
#include "cmd_args.h"
#include "akinator.h"

enum Error {
	AK_ERR	 = -5,
	FILE_ERR = -4,
	ARG_ERR  = -3,
	BUF_ERR  = -1,
	TRIO_ERR = -2,
	NO_ERR   =  0,
};

struct CmdArgs {
	const char *input_filename;
	const char *output_filename;
	const char *dump_filename;
	const char *log_filename;
	bool guess_mode; // enum
	bool comparison_mode;
	bool description_mode;
	bool do_speak;
};

enum ArgError handle_input_filename(const char *arg_str, void *processed_args);
enum ArgError handle_output_filename(const char *arg_str, void *processed_args);
enum ArgError handle_dump_filename(const char *arg_str, void *processed_args);
enum ArgError handle_log_filename(const char *arg_str, void *processed_args);
enum ArgError handle_guess_mode(const char *arg_str, void *processed_args);
enum ArgError handle_comparison_mode(const char *arg_str, void *processed_args);
enum ArgError handle_description_mode(const char *arg_str, void *processed_args);
enum ArgError handle_speaking_mode(const char *arg_str, void *processed_args);

void print_str(char *buf, const char *data, size_t n);

const struct ArgDef arg_defs[] = {
	{"input", 'i', "Name of the input database's file",
	 false, false, handle_input_filename},

	{"output", 'o', "Name of the output database's file. Optional: if not specified, database won't be saved",
	 true, false, handle_output_filename},

	{"dump", 'p', "Name of the html dump file. Optional: if not specified, dump won't be generated",
	 true, false, handle_dump_filename},

	{"log", 'l', "Name of the log file. Optional",
	 true, false, handle_log_filename},

	{"guess", '\0', "Enable guessing mode",
	 true, true, handle_guess_mode},

	{"compare", '\0', "Enable comparison mode",
	 true, true, handle_comparison_mode},

	{"describe", '\0', "Enable description mode",
	 true, true, handle_description_mode},

	{"speak", 's', "Enable speaking",
	 true, true, handle_speaking_mode},
};
const size_t ARG_DEFS_SIZE = sizeof(arg_defs) / sizeof(arg_defs[0]);
const size_t ERR_BUF_SIZE = 1024;

int main(int argc, const char *argv[])
{
	logger_ctor();
	add_log_handler({stderr, DEBUG, true});

	int ret_val = NO_ERR;

	struct CmdArgs args = { NULL, NULL, NULL, NULL, false, false, false };
	struct Buffer buf = {};
	struct Buffer ans_buf = {};
	struct Node *tr = NULL;

	char err_buf[ERR_BUF_SIZE] = {};
	enum ArgError arg_err = ARG_NO_ERR;
	enum BufferError buf_err = BUF_NO_ERR;
	enum TreeIOError trio_err = TRIO_NO_ERR;
	struct AkError ak_err = compose_err(AK_NO_ERR, "");

	FILE *save_file = NULL;
	FILE *dump_html = NULL;
	FILE *log_file = NULL;

	arg_err = process_args(arg_defs, ARG_DEFS_SIZE, argv, argc, &args);
	if (arg_err < 0) {
		log_message(ERROR, "Argument error: %s\n", arg_err_to_str(arg_err));
		arg_show_usage(arg_defs, ARG_DEFS_SIZE, argv[0]);
		ret_val = ARG_ERR;
		goto finally;
	} else if (arg_err == ARG_HELP_CALLED) {
		goto finally;
	}

	if (args.log_filename) {
		log_file = fopen(args.log_filename, "w");
		if (!log_file) {
			log_message(ERROR, "Couldn't read file %s\n", args.log_filename);
			ret_val = FILE_ERR;
			goto finally;
		}
		add_log_handler({log_file, DEBUG, false});
	}

	buf_err = buffer_ctor(&buf);
	if (buf_err < 0) {
		log_message(ERROR, "Buffer error: %s\n", buffer_err_to_str(buf_err));
		ret_val = BUF_ERR;
		goto finally;
	}
	buf_err = buffer_load_from_file(&buf, args.input_filename);
	if (buf_err < 0) {
		log_message(ERROR, "Couldn't read file %s: %s\n", args.input_filename,
					buffer_err_to_str(buf_err));
		ret_val = FILE_ERR;
		goto finally;
	}
	buf_err = buffer_ctor(&ans_buf);
	if (buf_err < 0) {
		log_message(ERROR, "Buffer error: %s\n", buffer_err_to_str(buf_err));
		ret_val = BUF_ERR;
		goto finally;
	}

	trio_err = tree_load_from_buf(&tr, &buf);
	if (trio_err < 0) {
		log_message(ERROR, "Tree input error: %s\n",
					tree_io_err_to_str(trio_err));
		ret_val = TRIO_ERR;
		goto finally;
	}

	if (args.dump_filename) {
		dump_html = tree_start_html_dump(args.dump_filename);
		if (!dump_html) {
			log_message(ERROR, "Couldn't read file %s\n", args.dump_filename);
			ret_val = FILE_ERR;
			goto finally;
		}
		TREE_DUMP_GUI(tr, dump_html, print_str);
	}

	if (args.guess_mode) {
		ak_err = guess(&tr, &ans_buf, args.do_speak);
	} else if (args.description_mode) {
		ak_err = describe(tr, args.do_speak);
	} else if (args.comparison_mode) {
		ak_err = compare(tr, args.do_speak);
	} else {
		log_message(ERROR, "Program mode wasn't specified\n");
		arg_show_usage(arg_defs, ARG_DEFS_SIZE, argv[0]);
		ret_val = ARG_ERR;
		goto finally;
	}

	if (ak_err.code < 0) {
		ak_err_to_str(err_buf, ak_err, ERR_BUF_SIZE);
		log_message(ERROR, "Akinator error: %s\n", err_buf);
		ret_val = AK_ERR;
		goto finally;
	}

	if (dump_html) //{
		//        +xx
		TREE_DUMP_GUI(tr, dump_html, print_str);

	if (args.output_filename) {
		save_file = fopen(args.output_filename, "w");
		if (!save_file) {
			log_message(ERROR, "Couldn't read file %s\n", args.output_filename);
			ret_val = FILE_ERR;
			goto finally;
		}
		// maybe some errors?
		tree_save(tr, save_file);
	}

	finally:
		node_op_delete(tr);
		buffer_dtor(&buf);
		buffer_dtor(&ans_buf);
		logger_dtor();
		if (save_file)
			fclose(save_file);
		if (dump_html)
			tree_end_html_dump(dump_html);
		if (log_file)
			fclose(log_file);

	return ret_val;
}

void print_str(char *buf, const char *data, size_t n)
{
	snprintf(buf, n, "%s", data);
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

enum ArgError handle_log_filename(const char *arg_str, void *processed_args)
{
	struct CmdArgs *args = (struct CmdArgs*) processed_args;
	args->log_filename = arg_str;
	return ARG_NO_ERR;
}

enum ArgError handle_guess_mode(const char */*arg_str*/, void *processed_args)
{
	struct CmdArgs *args = (struct CmdArgs*) processed_args;
	if (args->comparison_mode || args->description_mode)
		return ARG_WRONG_ARGS_ERR;
	args->guess_mode = true;
	return ARG_NO_ERR;
}

enum ArgError handle_comparison_mode(const char */*arg_str*/, void *processed_args)
{
	struct CmdArgs *args = (struct CmdArgs*) processed_args;
	if (args->guess_mode || args->description_mode)
		return ARG_WRONG_ARGS_ERR;
	args->comparison_mode = true;
	return ARG_NO_ERR;
}

enum ArgError handle_description_mode(const char */*arg_str*/, void *processed_args)
{
	struct CmdArgs *args = (struct CmdArgs*) processed_args;
	if (args->guess_mode || args->comparison_mode)
		return ARG_WRONG_ARGS_ERR;
	args->description_mode = true;
	return ARG_NO_ERR;
}

enum ArgError handle_speaking_mode(const char */*arg_str*/, void *processed_args)
{
	struct CmdArgs *args = (struct CmdArgs*) processed_args;
	args->do_speak = true;
	return ARG_NO_ERR;
}
