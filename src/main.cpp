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

void print_str(char *buf, const char *data, size_t n);

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