#ifndef OUTPUT_H_INCLUDED
#define OUTPUT_H_INCLUDED

#include <stdio.h>

#include "tracee.h"

struct options;

/*
 * Function prototype used for output formatting.
 */
typedef void (*output_fn_t)(FILE*, struct tracee*, struct options *options);

/*
 * The default output function.
 */
extern const output_fn_t default_output_fn;

/*
 * Get output function for the given format name.
 */
output_fn_t get_output_fn(const char *name);

/*
 * Get a NULL-terminated list of supported output formats.
 */
const char **get_output_formats(void);

/*
 * Tracee (and its children) should be excluded based on user-provided pattern.
 */
bool output_exclude(struct tracee *tracee, struct options *options);

#endif
