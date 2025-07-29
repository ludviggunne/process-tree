#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/ptrace.h>
#include <sys/syscall.h>

#include <linux/limits.h>
#include <linux/sched.h>

#include "tracee.h"
#include "xmalloc.h"

typedef unsigned long word_t;

struct tracee *tracee_create(void)
{
	return xcalloc(1, sizeof(struct tracee));
}

void tracee_destroy(struct tracee *tracee)
{
	free_string_list(tracee->argv);
	free_string_list(tracee->envp);

	for (size_t i = 0; i < tracee->nchildren; ++i)
	{
		tracee_destroy(tracee->children[i]);
	}

	xfree(tracee->children);
	xfree(tracee);
}

void tracee_detach(struct tracee *tracee)
{
	for (size_t i = 0; i < tracee->nchildren; ++i)
	{
		tracee_detach(tracee->children[i]);
	}

	(void) ptrace(PTRACE_DETACH, tracee->tid);
}

void tracee_add_child(struct tracee *parent, struct tracee *child)
{
	parent->nchildren++;
	parent->children = xrealloc(parent->children, sizeof(struct tracee *) * parent->nchildren);
	parent->children[parent->nchildren-1] = child;

	child->cwd = strdup(parent->cwd);

	if (parent->next_child_is_a_thread)
	{
		child->is_a_thread = true;
		parent->next_child_is_a_thread = false;
	}
}

struct tracee *tracee_find_tid(struct tracee *root, long tid)
{
	struct tracee *result = NULL;

	if (root->tid == tid)
	{
		return root;
	}

	for (size_t i = 0; i < root->nchildren; ++i)
	{
		if ((result = tracee_find_tid(root->children[i], tid)))
		{
			break;
		}
	}

	return result;
}

char *tracee_read_string(struct tracee *tracee, unsigned long addr)
{
	word_t *data = NULL;
	size_t offset = 0;
	unsigned long peekaddr;
	long tid = tracee->tid;

	if (addr == 0)
	{
		return NULL;
	}

	for (;; offset++)
	{
		data = xrealloc(data, sizeof(word_t) * (offset+1));
		peekaddr = addr + sizeof(word_t) * offset;

		errno = 0;
		data[offset] = ptrace(PTRACE_PEEKTEXT, tid, peekaddr);

		if (errno != 0)
		{
			/* TODO: handle error */
		}

		/* Check if last read word contains a NULL byte. */
		char *word = (char *) &data[offset];
		for (size_t i = 0; i < sizeof(word_t); ++i)
		{
			if (word[i] == 0)
			{
				goto done;
			}
		}
	}

done:
	return (char *) data;
}

char **tracee_read_string_list(struct tracee *tracee, unsigned long addr)
{
	char **data = NULL;
	size_t offset = 0;
	unsigned long peekaddr, straddr;
	long tid = tracee->tid;

	for (;; offset++)
	{
		data = xrealloc(data, sizeof(word_t) * (offset+1));
		peekaddr = addr + sizeof(word_t) * offset;

		errno = 0;
		straddr = ptrace(PTRACE_PEEKTEXT, tid, peekaddr);

		if (errno != 0)
		{
			/* TODO: handle error */
		}

		if (straddr == 0)
		{
			data[offset] = NULL;
			break;
		}

		data[offset] = tracee_read_string(tracee, straddr);
	}

	return data;
}

void tracee_chdir(struct tracee *tracee, const char *dir)
{
	struct tracee *child;

	xfree(tracee->cwd);
	tracee->cwd = strdup(dir);

	for (size_t i = 0; i < tracee->nchildren; ++i)
	{
		child = tracee->children[i];
		if (child->is_a_thread)
		{
			tracee_chdir(child, dir);
		}
	}
}

int tracee_get_syscall_info(struct tracee *tracee, struct ptrace_syscall_info *info)
{
	const size_t size = sizeof(struct ptrace_syscall_info);
	return ptrace(PTRACE_GET_SYSCALL_INFO, tracee->tid, size, info) == size ? 0 : -1;
}

long tracee_get_event_tid(struct tracee *tracee)
{
	long tid;

	if (ptrace(PTRACE_GETEVENTMSG, tracee->tid, 0, &tid) < 0)
	{
		tid = -1;
	}

	return tid;
}

static char **read_string_list_from_file(const char *path)
{
	FILE *f;
	char *word = NULL;
	size_t wordsize = 0;
	char **list = NULL;
	size_t length = 0;

	f = fopen(path, "r");

	if (f == NULL)
	{
		return NULL;
	}

	while (getdelim(&word, &wordsize, '\0', f) >= 0)
	{
		length++;
		list = xrealloc(list, sizeof(*list) * length);
		list[length-1] = word;
		word = NULL;
	}

	fclose(f);

	length++;
	list = xrealloc(list, sizeof(*list) * length);
	list[length-1] = NULL;

	return list;
}

int tracee_read_info_from_proc_dir(struct tracee *tracee)
{
	char cwd_path[PATH_MAX];
	char cmdline_path[PATH_MAX];
	char environ_path[PATH_MAX];
	char cwd_buf[PATH_MAX];
	char **argv = NULL;
	char **envp = NULL;
	long tid = tracee->tid;

	snprintf(cwd_path, sizeof(cwd_path), "/proc/%ld/cwd", tid);
	snprintf(cmdline_path, sizeof(cmdline_path), "/proc/%ld/cmdline", tid);
	snprintf(environ_path, sizeof(environ_path), "/proc/%ld/environ", tid);

	if (readlink(cwd_path, cwd_buf, sizeof(cwd_buf)) < 0)
	{
		return -1;
	}

	argv = read_string_list_from_file(cmdline_path);
	envp = read_string_list_from_file(environ_path);

	if (argv == NULL || envp == NULL)
	{
		xfree(argv);
		xfree(envp);
		return -1;
	}

	free_string_list(tracee->argv);
	free_string_list(tracee->argv);
	xfree(tracee->cwd);

	tracee->argv = argv;
	tracee->envp = envp;
	tracee->cwd = strdup(cwd_buf);

	return 0;
}

int tracee_set_cwd_from_chdir_call(struct tracee *tracee, struct ptrace_syscall_info *info)
{
	assert(info->op == PTRACE_SYSCALL_INFO_ENTRY);
	assert(info->entry.nr == SYS_chdir);

	xfree(tracee->cwd);
	tracee->cwd = tracee_read_string(tracee, info->entry.args[0]);

	return 0;
}

int tracee_set_ptrace_options(struct tracee *tracee)
{
	if (tracee->ptrace_options_set)
	{
		return 0;
	}

	static const unsigned long options =
		PTRACE_O_TRACEEXEC | PTRACE_O_TRACEFORK |
		PTRACE_O_TRACEVFORK | PTRACE_O_TRACECLONE |
		PTRACE_O_TRACESYSGOOD;

	int result = ptrace(PTRACE_SETOPTIONS, tracee->tid, 0, options);
	tracee->ptrace_options_set = result == 0;
	return result;
}

int tracee_read_cl_args(struct tracee *tracee, struct ptrace_syscall_info *info)
{
	unsigned long cl_args_addr;
	unsigned long flags_addr;
	unsigned long flags;
	struct clone_args *cl_args;

	assert(info->op == PTRACE_SYSCALL_INFO_ENTRY);
	assert(info->entry.nr == SYS_clone3);

	cl_args_addr = info->entry.args[0];
	cl_args = (struct clone_args *) cl_args_addr;
	flags_addr = (unsigned long) &cl_args->flags;

	errno = 0;
	flags = ptrace(PTRACE_PEEKTEXT, tracee->tid, flags_addr);

	if (errno != 0)
	{
		return -1;
	}

	tracee->next_child_is_a_thread = flags & CLONE_THREAD;

	return 0;
}

char **copy_string_list(char **list)
{
	size_t length = 0;
	char **ptr = list;
	char **newlist;

	if (list == NULL)
	{
		return NULL;
	}

	while (*ptr)
	{
		++ptr;
		++length;
	}

	newlist = xmalloc(sizeof(*newlist) * (length+1));

	for (size_t i = 0; i < length; ++i)
	{
		newlist[i] = strdup(list[i]);
	}

	newlist[length] = NULL;
	return newlist;
}

void free_string_list(char **list)
{
	for (char **ptr = list; ptr && *ptr; ++ptr)
	{
		xfree(*ptr);
	}

	xfree(list);
}
