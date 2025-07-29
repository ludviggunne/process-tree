#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "options.h"

#define fatal_error(fmt, ...)                                                    \
	{                                                                        \
		usage(options, stderr);                                          \
		fprintf(stderr, "%s: " fmt, options->program_name, __VA_ARGS__); \
		exit(EXIT_FAILURE);                                              \
	}

static void usage(struct options *options, FILE *f)
{
	fprintf(f,
	        "Usage: %s [options...] [args...]\n"
	        "Options:\n"
	        "    -h/--help                Display this help message.\n"
	        "    -a/--attach <pid>        Attach to a running process.\n"
	        "    -o/--output <file>       Write output to <file>.\n"
	        "    -e/--exclude <pattern>   Exclude processes with arguments matching regular expression <pattern>.\n"
	        "    -s/--silent              Redirect child processes stdout and stderr to /dev/null.\n"
	        "    -r/--redirect            Redirect child processes stdout to stderr.\n"
	        "    -n/--no-env              Exclude environment from output.\n"
	        "    -f/--format <format>     Specify output format. May be one of:\n",
	        options->program_name);

	const char **formats = get_output_formats();
	for (; *formats; ++formats)
	{
		fprintf(f, "                              * %s\n", *formats);
	}
}

static void parse_attach_option(struct options *options, char *arg)
{
	char *endptr;

	errno = 0;
	options->attach = strtol(arg, &endptr, 10);

	if (errno != 0 || options->attach <= 0 || *endptr != 0)
	{
		fatal_error("Invalid pid: %s\n", arg);
	}
}

static void parse_format_option(struct options *options, char *arg)
{
	options->output_fn = get_output_fn(arg);

	if (options->output_fn == NULL)
	{
		fatal_error("Invalid output format '%s'\n", arg);
	}
}

static void parse_output_option(struct options *options, char *arg)
{
	options->outfile = fopen(arg, "w");
	if (options->outfile == NULL)
	{
		fatal_error("Failed to open output file %s: %s\n",
		            arg, strerror(errno));
	}
}

static void parse_exclude_option(struct options *options, char *arg)
{
	if (regcomp(&options->exclude, arg, 0) != 0)
	{
		fatal_error("Invalid regular expression: '%s'\n", arg);
	}

	options->has_exclude = true;
}

static void require_argument(struct options *options, char **argv, int *i)
{
	if (argv[*i+1] == NULL)
	{
		fatal_error("Option %s requires an argument\n", argv[*i]);
	}

	(*i)++;
}

void options_parse_cmdline(struct options *options, int argc, char **argv)
{
	memset(options, 0, sizeof(*options));

	options->program_name = argv[0];
	options->output_fn = default_output_fn;
	options->outfile = stdout;

	for (int i = 1; i < argc; ++i)
	{
		if (strcmp("-a", argv[i]) == 0 || strcmp("--attach", argv[i]) == 0)
		{
			require_argument(options, argv, &i);
			parse_attach_option(options, argv[i]);
			continue;
		}

		if (strcmp("-f", argv[i]) == 0 || strcmp("--format", argv[i]) == 0)
		{
			require_argument(options, argv, &i);
			parse_format_option(options, argv[i]);
			continue;
		}

		if (strcmp("-e", argv[i]) == 0 || strcmp("--exclude", argv[i]) == 0)
		{
			require_argument(options, argv, &i);
			parse_exclude_option(options, argv[i]);
			continue;
		}

		if (strcmp("-h", argv[i]) == 0 || strcmp("--help", argv[i]) == 0)
		{
			usage(options, stdout);
			exit(EXIT_SUCCESS);
		}

		if (strcmp("-s", argv[i]) == 0 || strcmp("--silent", argv[i]) == 0)
		{
			options->silent = true;
			continue;
		}

		if (strcmp("-r", argv[i]) == 0 || strcmp("--redirect", argv[i]) == 0)
		{
			options->redirect = true;
			continue;
		}

		if (strcmp("-n", argv[i]) == 0 || strcmp("--no-env", argv[i]) == 0)
		{
			options->exclude_environ = true;
			continue;
		}

		if (strcmp("-o", argv[i]) == 0 || strcmp("--output", argv[i]) == 0)
		{
			require_argument(options, argv, &i);
			parse_output_option(options, argv[i]);
			continue;
		}

		if (argv[i][0] == '-')
		{
			fatal_error("Invalid option: %s\n", argv[i]);
		}

		options->command = &argv[i];
		break;
	}

	if (options->attach && options->command)
	{
		usage(options, stderr);
		fprintf(stderr, "%s: Both command and external pid provided\n", options->program_name);
		exit(EXIT_FAILURE);
	}

	if (!options->attach && !options->command)
	{
		usage(options, stderr);
		fprintf(stderr, "%s: Neither command nor external pid provided\n", options->program_name);
		exit(EXIT_FAILURE);
	}
}
