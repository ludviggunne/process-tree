#ifndef TRACEE_H_INCLUDED
#define TRACEE_H_INCLUDED

#include <stddef.h>
#include <stdbool.h>

#include <linux/ptrace.h>

/*
 * Represents a process that is or has been traced.
 */
struct tracee
{
	/* Thread ID of this tracee. */
	long tid;
	
	/* Command line argument list, or NULL if this
	   tracee resulted from fork/vfork/clone and
	   execvp has not been executed. */
	char **argv;

	/* Environment variable list, or NULL if this
	   tracee resulted from fork/vfork/clone and
	   execvp has not been executed. */
	char **envp;

	/* Number of child processes/threads of this tracee. */
	size_t nchildren;

	/* A list of child processes/threads of this tracee. */
	struct tracee **children;

	/* Last working directory of this tracee. */
	char *cwd;

	/* Ptrace options have been set for this tracee. */
	bool ptrace_options_set;

	/* This tracee is a thread. */
	bool is_a_thread;

	/* The next child to be registered for this tracee is a thread. */
	bool next_child_is_a_thread;
};

/*
 * Allocate a new tracee with all fields set to zero.
 */
struct tracee *tracee_create(void);

/*
 * Free memory used by tracee and all children.
 */
void tracee_destroy(struct tracee *tracee);

/*
 * Detach from tracee and all children.
 */
void tracee_detach(struct tracee *tracee);

/*
 * Add a child tracee to the parent tracee.
 */
void tracee_add_child(struct tracee *parent, struct tracee *child);

/*
 * Find the tracee matching the provided tid in the tree.
 */
struct tracee *tracee_find_tid(struct tracee *root, long tid);

/*
 * Read a string from tracee at the specified address.
 */
char *tracee_read_string(struct tracee *tracee, unsigned long addr);

/*
 * Read a NULL terminated list of strings from tracee at the specified address.
 */
char **tracee_read_string_list(struct tracee *tracee, unsigned long addr);

/*
 * Change the working directory of the tracee and all non-thread children.
 */
void tracee_chdir(struct tracee *tracee, const char *dir);

/*
 * Get working directory, environment and command line arguments from /proc/<pid>.
 * Used when attaching to an external process.
 * Returns 0 on success and -1 on failure, errno is set by the
 * corresponding libc call.
 */
int tracee_read_info_from_proc_dir(struct tracee *tracee);

/*
 * Get command line arguments from execve arguments.
 */
int tracee_set_argv_from_execve_call(struct tracee *tracee, struct ptrace_syscall_info *info);

/*
 * Get environment from execve arguments.
 */
int tracee_set_envp_from_execve_call(struct tracee *tracee, struct ptrace_syscall_info *info);

/*
 * Get working directory from chdir arguments.
 */
int tracee_set_cwd_from_chdir_call(struct tracee *tracee, struct ptrace_syscall_info *info);

 /*
  * Get syscall info if tracee stopped from a syscall.
  * Returns 0 on success and -1 on failure, errno is
  * set by the corresponding ptrace call.
  */
int tracee_get_syscall_info(struct tracee *tracee, struct ptrace_syscall_info *info);

 /*
  * Get the returned tid tracee stopped from an event coming from fork, vfork or clone.
  * Returns -1 on failure, errno is set by the corresponding ptrace call.
  */
long tracee_get_event_tid(struct tracee *tracee);

/*
 * Set the appropriate ptrace options for this tracee.
 * Returns 0 on success and -1 on failure, errno is set
 * by the corresponding ptrace call.
 */
int tracee_set_ptrace_options(struct tracee *tracee);

/* Set `next_child_is_thread` field based on the `cl_args` arugment to clone3.
 * Returns 0 on success and -1 on failure, errno is set
 * by the corresponding ptrace call.
 */
int tracee_read_cl_args(struct tracee *tracee, struct ptrace_syscall_info *info);

/*
 * Copy a NULL terminated list of strings.
 */
char **copy_string_list(char **list);

/*
 * Free a NULL terminated list of strings.
 */
void free_string_list(char **list);

#endif
