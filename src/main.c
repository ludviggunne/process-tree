#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/syscall.h>

#include <linux/limits.h>

#include "options.h"
#include "tracee.h"
#include "status.h"

extern char **environ;

static struct tracee *create_root_tracee_from_command(char **command);
static struct tracee *create_root_tracee_with_attach(long pid);

static void handle_exit(struct tracee *tracee);
static void handle_syscall(struct tracee *tracee);
static void handle_new_tracee(int status, struct tracee *tracee);
static void continue_tracee(long tid);

static void cleanup(void);
static void sigint_handler(int);

static struct options options = {0};
static struct tracee *root = NULL;

static char **execve_argv = NULL;
static char **execve_envp = NULL;

int main(int argc, char **argv)
{
	options_parse_cmdline(&options, argc, argv);

	if (options.command)
	{
		root = create_root_tracee_from_command(options.command);
	}
	else
	{
		root = create_root_tracee_with_attach(options.attach);
	}

	atexit(cleanup);
	signal(SIGINT, sigint_handler);

	for (;;)
	{
		struct tracee *tracee;
		int status;
		long tid;

		tid = wait(&status);

		tracee = tracee_find_tid(root, tid);

		if (tracee == NULL)
		{
			continue_tracee(tid);
			continue;
		}

		if (WIFEXITED(status) || status_is_exit_event(status))
		{
			handle_exit(tracee);
			continue;
		}

		if (!tracee->ptrace_options_set)
		{
			(void) tracee_set_ptrace_options(tracee);
		}

		bool is_new_tracee = status_is_clone_event(status)
		                  || status_is_fork_event(status)
		                  || status_is_vfork_event(status);

		if (is_new_tracee)
		{
			handle_new_tracee(status, tracee);
			continue_tracee(tid);
			continue;
		}

		if (status_is_syscall(status))
		{
			handle_syscall(tracee);
			continue_tracee(tid);
			continue;
		}

		if (status_is_execve_event(status))
		{
			tracee->argv = execve_argv;
			tracee->envp = execve_envp;

			execve_argv = NULL;
			execve_envp = NULL;

			continue_tracee(tid);
			continue;
		}


		continue_tracee(tid);
	}
}

static void cleanup(void)
{
	if (root)
	{
		tracee_detach(root);
	}

	options.output_fn(options.outfile, root, &options);
}

static void sigint_handler(int sig)
{
	exit(EXIT_SUCCESS);
}

static void continue_tracee(long tid)
{
	if (ptrace(PTRACE_SYSCALL, tid, 0, 0) < 0)
	{
		fprintf(stderr, "%s: ptrace(PTRACE_SYSCALL, %ld) failed: %s\n",
		        options.program_name, tid, strerror(errno));
		exit(EXIT_FAILURE);
	}
}

static struct tracee *create_root_tracee_from_command(char **command)
{
	char cwdbuf[PATH_MAX];
	long pid;
	struct tracee *root;

	pid = fork();

	if (pid < 0)
	{
		fprintf(stderr, "%s: Failed to fork process: %s\n",
		        options.program_name, strerror(errno));
		exit(EXIT_FAILURE);
	}

	if (pid > 0)
	{
		root = tracee_create();

		root->tid = pid;
		root->argv = copy_string_list(command);
		root->envp = copy_string_list(environ);
		root->cwd = strdup(getcwd(cwdbuf, sizeof(cwdbuf)));

		return root;
	}

	if (ptrace(PTRACE_TRACEME) < 0)
	{
		fprintf(stderr, "%s: ptrace(PTRACE_TRACEME) failed: %s\n",
		        options.program_name, strerror(errno));
		exit(EXIT_FAILURE);
	}

	if (options.silent)
	{
		FILE *devnull = fopen("/dev/null", "a");

		if (devnull == NULL)
		{
			fprintf(stderr, "%s: Failed to open /dev/null: %s\n",
			        options.program_name, strerror(errno));
			exit(EXIT_FAILURE);
		}

		if (dup2(fileno(devnull), STDOUT_FILENO) < 0)
		{
			fprintf(stderr, "%s: Failed to redirect stdout to /dev/null: %s\n",
			        options.program_name, strerror(errno));
			exit(EXIT_FAILURE);
		}

		if (dup2(fileno(devnull), STDERR_FILENO) < 0)
		{
			fprintf(stderr, "%s: Failed to redirect stderr to /dev/null: %s\n",
			        options.program_name, strerror(errno));
			exit(EXIT_FAILURE);
		}
	}
	else if (options.redirect)
	{
		if (dup2(STDERR_FILENO, STDOUT_FILENO) < 0)
		{
			fprintf(stderr, "%s: Failed to redirect stderr to /dev/null: %s\n",
			        options.program_name, strerror(errno));
			exit(EXIT_FAILURE);
		}
	}

	if (execvp(command[0], command) < 0)
	{
		fprintf(stderr, "%s: Failed to execute %s: %s\n",
		        options.program_name, command[0], strerror(errno));
		exit(EXIT_FAILURE);
	}

	__builtin_unreachable();
}

static struct tracee *create_root_tracee_with_attach(long pid)
{
	struct tracee *root;

	if (ptrace(PTRACE_ATTACH, pid) < 0)
	{
		fprintf(stderr, "%s: Failed to attach to process %ld: %s\n",
		        options.program_name, pid, strerror(errno));
		exit(EXIT_FAILURE);
	}

	root = tracee_create();
	root->tid = pid;

	if (tracee_read_info_from_proc_dir(root) < 0)
	{
		fprintf(stderr, "%s: Failed to get info about root tracee %ld: %s\n",
		        options.program_name, pid, strerror(errno));
		exit(EXIT_FAILURE);
	}

	return root;
}

static void handle_exit(struct tracee *tracee)
{
	if (tracee == root)
	{
		exit(EXIT_SUCCESS);
	}
}

static void handle_syscall(struct tracee *tracee)
{
	struct ptrace_syscall_info info;
	(void) tracee_get_syscall_info(tracee, &info);

	if (info.op != PTRACE_SYSCALL_INFO_ENTRY)
	{
		return;
	}

	switch (info.entry.nr)
	{
	case SYS_execve:
		execve_argv = tracee_read_string_list(tracee, info.entry.args[1]);
		execve_envp = tracee_read_string_list(tracee, info.entry.args[2]);
		break;

	case SYS_chdir:
		(void) tracee_set_cwd_from_chdir_call(tracee, &info);
		break;

	case SYS_clone3:
		(void) tracee_read_cl_args(tracee, &info);
		break;

	default:
		break;
	}
}

static void handle_new_tracee(int status, struct tracee *tracee)
{
	long newtid;
	struct tracee *newtracee;

	newtid = tracee_get_event_tid(tracee);
	if (newtid < 0)
	{
		fprintf(stderr, "%s: ptrace(PTRACE_GETEVENTMSG, %ld, ...) failed: %s\n",
		        options.program_name, tracee->tid, strerror(errno));
		exit(EXIT_FAILURE);
	}

	newtracee = tracee_create();
	newtracee->tid = newtid;

	tracee_add_child(tracee, newtracee);
}

