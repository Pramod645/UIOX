| File | Concept from source |
| --- | --- |
| ipc_types.h | Shared IPC constants, IpcPerm, SimProcess, sleep/wakeup stubs |
| ptrace.h/c | Process tracing — PTRACE_TRACEME, PEEKDATA/POKEDATA (4-switch transfer), CONT, KILL, GETREGS/SETREGS |
| msg.h/c | Algorithms 1–4 — msgget, msgctl, msgsnd, msgrcv (typed message selection, sleep/wakeup) |
| shm.h/c | Algorithms 5–8 — shmget, shmat (lazy page allocation on first attach), shmdt (data persists), shmctl |
| sem.h/c | Algorithms 9–11 — semget, semctl, semop (atomic P/V, reverse-on-block, UNDO flag, wait_incr/wait_zero) |
| socket.h/c | Three-layer model (socket/protocol/device) — socket, bind, listen, connect, accept, send/recv, sendto/recvfrom, OOB, peek, shutdown, close, getsockname, getsockopt/setsockopt |