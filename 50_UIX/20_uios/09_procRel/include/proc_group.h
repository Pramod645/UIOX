#ifndef PROC_GROUP_H
#define PROC_GROUP_H

/*
 * proc_group.h — Process group functions.
 *
 * APUE Chapter 9, Section 9.4
 *
 * A process group is a collection of one or more processes,
 * usually associated with the same job, that can receive
 * signals from the same terminal.
 *
 * Each process group has a unique process group ID (PGID),
 * stored as a pid_t.
 *
 * Process Group Leader:
 *   The leader is identified when process group ID == process ID.
 *   A leader can create a group, fork children into it, and
 *   then terminate — the group persists as long as any member
 *   remains.
 *
 * Key functions:
 *   getpgrp()   — return PGID of calling process (POSIX)
 *   getpgid(0)  — equivalent to getpgrp()
 *   setpgid()   — join or create a process group
 *
 * setpgid() rules:
 *   1. A process can only change itself or its children.
 *   2. Cannot change a child's PGID after child called exec.
 *   3. If pid==pgid, process becomes group leader.
 *   4. If pid==0, use calling process's PID.
 *   5. If pgid==0, use pid as the new PGID.
 *
 * Race condition avoidance:
 *   Job-control shells call setpgid in BOTH parent and child
 *   after fork — one call is redundant but ensures the child
 *   is placed into its group before either process proceeds.
 */

#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/*
 * pg_print_info()
 *
 * Prints PID, PPID, PGID, and SID for the current process.
 * Useful for verifying process group membership.
 *
 * @param label  String prefix for the output line.
 */
void pg_print_info(const char *label);

/*
 * pg_create_new_group()
 *
 * Makes the calling process a process group leader by setting
 * its PGID to its own PID via setpgid(0, 0).
 *
 * Returns 0 on success, -1 on error.
 */
int pg_create_new_group(void);

/*
 * pg_join_group()
 *
 * Places the process identified by pid into the process group
 * identified by pgid.
 *
 * @param pid   Process to move (0 = self).
 * @param pgid  Target process group (0 = use pid's PID).
 *
 * Returns 0 on success, -1 on error.
 */
int pg_join_group(pid_t pid, pid_t pgid);

/*
 * pg_is_leader()
 *
 * Returns 1 if the calling process is a process group leader
 * (PID == PGID), 0 otherwise.
 */
int pg_is_leader(void);

/*
 * pg_demo_fork_setpgid()
 *
 * Demonstrates the race-condition-free way to place a forked
 * child into its own process group:
 *   1. Parent calls setpgid(child_pid, child_pid).
 *   2. Child calls setpgid(0, 0).
 *   Both calls are made — one is redundant, but atomicity
 *   is guaranteed regardless of scheduling order.
 */
void pg_demo_fork_setpgid(void);

#endif /* PROC_GROUP_H */
