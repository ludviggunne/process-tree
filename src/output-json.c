#include <string.h>
#include <stdio.h>

#include "tracee.h"
#include "output.h"
#include "options.h"

static void fputs_escaped(FILE *f, const char *str, int len)
{
	for (int i = 0; i < len; ++i)
	{
		char c = str[i];
		switch (c)
		{
		case '\\':
			fputs("\\\\", f);
			break;
		case '\"':
			fputs("\\\"", f);
			break;
		case '\n':
			fputs("\\n", f);
			break;
		case '\r':
			fputs("\\r", f);
			break;
		case '\t':
			fputs("\\t", f);
			break;
		default:
			fputc(c, f);
			break;
		}
	}
}

void output_fn_json(FILE *f, struct tracee *tracee, struct options *options)
{
	char **ptr;
	bool first;

	fprintf(f, "{");

	fprintf(f, "\"tid\":%ld", tracee->tid);

	if (tracee->cwd)
	{
		fprintf(f, ",\"directory\":\"");
		fputs_escaped(f, tracee->cwd, strlen(tracee->cwd));
		fprintf(f, "\"");
	}

	if (tracee->argv)
	{
		fprintf(f, ",\"arguments\":[");

		first = true;
		for (ptr = tracee->argv; *ptr; ++ptr)
		{
			const char *comma = first ? "" : ",";
			first = false;

			fprintf(f, "%s\"", comma);
			fputs_escaped(f, *ptr, strlen(*ptr));
			fprintf(f, "\"");
		}

		fprintf(f, "]");
	}

	if (tracee->envp)
	{
		fprintf(f, ",\"environment\":{");

		first = true;
		for (ptr = tracee->envp; *ptr; ++ptr)
		{
			const char *eq = strchr(*ptr, '=');

			if (eq == NULL)
			{
				continue;
			}

			const char *key = *ptr;
			int keylen = eq-key;
			const char *value = eq+1;

			const char *comma = first ? "" : ",";
			first = false;


			fprintf(f, "%s\"", comma);
			fputs_escaped(f, key, keylen);
			fprintf(f, "\":\"");
			fputs_escaped(f, value, strlen(value));
			fprintf(f, "\"");
		}

		fprintf(f, "}");
	}

	if (tracee->children)
	{
		fprintf(f, ",\"children\":[");

		first = true;
		for (int i = 0; i < tracee->nchildren; ++i)
		{
			if (output_exclude(tracee->children[i], options))
			{
				continue;
			}

			if (!first)
			{
				fprintf(f, ",");
			}

			first = false;

			output_fn_json(f, tracee->children[i], options);
		}

		fprintf(f, "]");
	}

	fprintf(f, "}");
}
