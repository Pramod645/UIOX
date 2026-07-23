/*
 * 30_KIX/33_PCS/50_scps/include/uiox_sys_call_nr.h
 *
 * UIOX syscall numbers — single source of truth for kernel and user-space.
 * Keep in sync with the table in uiox_sys_call.c.
 *
 * @version 2.0.0  @date 2026-07-23
 */
#ifndef UIOX_SYS_CALL_NR_H
#define UIOX_SYS_CALL_NR_H

#define UIOX_SYS_NR_EXIT            1u
#define UIOX_SYS_NR_FORK            2u
#define UIOX_SYS_NR_READ            3u
#define UIOX_SYS_NR_WRITE           4u
#define UIOX_SYS_NR_OPEN            5u
#define UIOX_SYS_NR_CLOSE           6u
#define UIOX_SYS_NR_WAIT_PID        7u
#define UIOX_SYS_NR_EXECVE          8u
#define UIOX_SYS_NR_GET_PID         9u
#define UIOX_SYS_NR_GET_PPID       10u
#define UIOX_SYS_NR_BRK            11u
#define UIOX_SYS_NR_MMAP           12u
#define UIOX_SYS_NR_MUNMAP         13u
#define UIOX_SYS_NR_NANO_SLEEP     14u
#define UIOX_SYS_NR_CLOCK_GET_TIME 15u
#define UIOX_SYS_NR_KILL           16u
#define UIOX_SYS_NR_SIG_ACTION     17u
#define UIOX_SYS_NR_MAX            64u

#endif /* UIOX_SYS_CALL_NR_H */
