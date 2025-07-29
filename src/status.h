#ifndef STATUS_H_INCLUDED
#define STATUS_H_INCLUDED

#include <stdbool.h>

/*
 * These macros are used to determine the reason why a
 * tracee stopped, based on the status retrieved from wait().
 */

#define status_is_syscall(status) (WIFSTOPPED(status) && WSTOPSIG(status) == (SIGTRAP | 0x80))
#define status_is_exit_event(status) (WIFSTOPPED(status) && ((status) >> 8 == (SIGTRAP | (PTRACE_EVENT_EXIT << 8))))
#define status_is_fork_event(status) (WIFSTOPPED(status) && ((status) >> 8 == (SIGTRAP | (PTRACE_EVENT_FORK << 8))))
#define status_is_vfork_event(status) (WIFSTOPPED(status) && ((status) >> 8 == (SIGTRAP | (PTRACE_EVENT_VFORK << 8))))
#define status_is_clone_event(status) (WIFSTOPPED(status) && ((status) >> 8 == (SIGTRAP | (PTRACE_EVENT_CLONE << 8))))
#define status_is_execve_event(status) (WIFSTOPPED(status) && ((status) >> 8 == (SIGTRAP | (PTRACE_EVENT_EXEC << 8))))

#endif
