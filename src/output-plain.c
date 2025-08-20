#include <stdio.h>

#include "tracee.h"
#include "options.h"
#include "output.h"

void output_fn_plain(FILE *f, struct tracee *tracee, struct options *options)
{
	if (output_exclude(tracee, options) || !tracee->argv)
	{
		return;
	}

	bool first_arg = true;

	for (char **arg = tracee->argv; *arg; ++arg)
	{
		if (first_arg)
		{
			fprintf(f, "%s", *arg);
		}
		else
		{
			fprintf(f, " %s", *arg);
		}

		first_arg = false;
	}

	fputc('\n', f);

	for (size_t i = 0; i < tracee->nchildren; ++i)
	{
		struct tracee *child = tracee->children[i];
		output_fn_plain(f, child, options);
	}
}
