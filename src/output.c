#include <string.h>

#include "output.h"
#include "options.h"

void output_fn_tree(FILE*, struct tracee*, struct options*);
void output_fn_json(FILE*, struct tracee*, struct options*);
void output_fn_plain(FILE*, struct tracee*, struct options*);

typedef struct {
	const char *name;
	output_fn_t fn;
} output_fn_entry_t;

#define output_fn_entry(fn) { #fn, output_fn_ ## fn }

static const output_fn_entry_t output_fns[] = {
	output_fn_entry(tree),
	output_fn_entry(json),
	output_fn_entry(plain),
	{0},
};

output_fn_t get_output_fn(const char *name)
{
	const output_fn_entry_t *entry;

	for (entry = output_fns; entry->name; ++entry)
	{
		if (strcasecmp(entry->name, name) == 0)
		{
			return entry->fn;
		}
	}

	return NULL;
}

const output_fn_t default_output_fn = output_fn_tree;

static const char *formats[sizeof(output_fns) / sizeof(output_fns[0])];

const char **get_output_formats(void)
{
	const output_fn_entry_t *entry;
	size_t i;

	for (entry = output_fns, i = 0; entry->name; ++entry, ++i)
	{
		formats[i] = entry->name;
	}

	formats[i] = NULL;
	return formats;
}

bool output_exclude(struct tracee *tracee, struct options *options)
{
	regmatch_t match;

	if (!options->has_exclude || !tracee->argv)
	{
		return false;
	}

	for (char **arg = tracee->argv; *arg; ++arg)
	{
		if (regexec(&options->exclude, *arg, 1, &match, 0) == 0)
		{
			return true;
		}
	}

	return false;
}
