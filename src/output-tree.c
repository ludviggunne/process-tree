#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include "tracee.h"
#include "output.h"

static void output_fn_tree_rec(FILE *f, struct tracee *tracee, struct options *options, bool *prefix, size_t indent)
{
	struct tracee *child;
	char **argv;

	size_t last_child = 0;
	for (size_t i = 0; i < tracee->nchildren; ++i)
	{
		if (!output_exclude(tracee->children[i], options))
		{
			last_child = i;
		}
	}

	for (size_t i = 0; tracee->children && i <= last_child; ++i)
	{
		child = tracee->children[i];

		if (output_exclude(child, options))
		{
			continue;
		}

		for (size_t j = 0; j < indent; ++j)
		{
			if (prefix[j])
			{
				fprintf(f, "│   ");
			}
			else
			{
				fprintf(f, "    ");
			}
		}

		if (i < last_child)
		{
			prefix[indent] = true;
			fprintf(f, "├───");
		}
		else
		{
			prefix[indent] = false;
			fprintf(f, "└───");
		}

		argv = child->argv;

		if (argv)
		{
			for (; *argv; ++argv)
			{
				fprintf(f, "%s ", *argv);
			}
		}
		else
		{
			fprintf(f, "%ld ", child->tid);
		}

		fprintf(f, "\n");
		output_fn_tree_rec(f, child, options, prefix, indent+1);
	}
}

void output_fn_tree(FILE *f, struct tracee *tracee, struct options *options)
{
	bool prefix[4096] = {0};
	size_t indent = 0;

	if (output_exclude(tracee, options))
	{
		return;
	}

	for (char **arg = tracee->argv; *arg; ++arg)
	{
		fprintf(f, "%s ", *arg);
	}

	fprintf(f, "\n");

	output_fn_tree_rec(f, tracee, options, prefix, indent);
}
