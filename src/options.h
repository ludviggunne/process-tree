#ifndef OPTIONS_H_INCLUDED
#define OPTIONS_H_INCLUDED

#include <regex.h>

#include "output.h"

/*
 * Result of parsing command line arguments.
 */
struct options {

	/* The name of this program as invoked on the command line. */
	const char *program_name;

	/* Non-zero if the user provided a pid for an external process. */
	long attach;

	/* Non-NULL, NULL terminated list of strings if the user provided a command. */
	char **command;

	/* The function used for output formatting. */
	output_fn_t output_fn;

	/* File pointer for output */
	FILE *outfile;

	/* Regular expression for excluding branches in the process tree. */
	regex_t exclude;

	/* An exclude pattern was provided. */
	bool has_exclude;

	/* Redirect stdout and stderr to /dev/null. */
	bool silent;

	/* Redirect stdout to stderr. */
	bool redirect;

	/* Exclude environment from output. */
	bool exclude_environ;
};

/*
 * Parse command line arguments.
 */
void options_parse_cmdline(struct options *options, int argc, char **argv);

#endif
