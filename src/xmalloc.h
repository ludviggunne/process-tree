#ifndef XMALLOC_H_INCLUDED
#define XMALLOC_H_INCLUDED

#include <stdlib.h>
#include <assert.h>

#define malloc_wrapper(fn, ...)                                         \
	({                                                              \
		void *ptr = fn(__VA_ARGS__);                            \
		if (!ptr)                                               \
		{                                                       \
			fprintf(stderr, "Fatal error: %s failed", #fn); \
			exit(1);                                        \
		}                                                       \
		ptr;                                                    \
	})

#define xmalloc(size) malloc_wrapper(malloc, size)
#define xcalloc(nmemb, size) malloc_wrapper(calloc, nmemb, size)
#define xrealloc(ptr, size) malloc_wrapper(realloc, ptr, size)
#define xfree free

#endif
