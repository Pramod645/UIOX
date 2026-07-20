#include <stdio.h>
#include <string.h>
#include "ipc_types.h"
#include "ptrace.h"
#include "msg.h"
#include "shm.h"
#include "sem.h"
#include "socket.h"
#include <stdlib.h>

static void banner(const char *s)
{
    printf("\n══════════════════════════════════════════\n");
    printf("  %s\n", s);
    printf("══════════════════════════════════════════\n");
}

int main(void)
{
    /* ── 1. ptrace ───────────────────────────────────────────── */
    banner("Process Tracing (ptrace)");
    ptrace_init();

    SimProcess debugger = {.pid=1, .uid=1000, .gid=1000};
    SimProcess *tracee  = ptrace_spawn_child(&debugger, "a.out");

    uint64_t word = 0;
    ptrace(PTRACE_PEEKDATA,  &debugger, tracee, 10,  &word);
    word = 0xDEAD;
    ptrace(PTRACE_POKEDATA,  &debugger, tracee, 10,  &word);
    ptrace(PTRACE_PEEKDATA,  &debugger, tracee, 10,  &word);
    printf("  word at addr 10 = 0x%llx\n", (unsigned long long)word);

    uint64_t pc = 0x401000;
    ptrace(PTRACE_SETREGS,   &debugger, tracee, 0,   &pc);
    ptrace(PTRACE_GETREGS,   &debugger, tracee, 0,   &pc);
    ptrace(PTRACE_CONT,      &debugger, tracee, 0,   NULL);
    ptrace(PTRACE_KILL,      &debugger, tracee, 0,   NULL);
    free(tracee);

    /* ── 2. Message queues ──────────────────────────────────── */
    banner("Message Queues (msgget/msgsnd/msgrcv)");
    msg_init();

    SimProcess sender   = {.pid=10};
    SimProcess receiver = {.pid=11};

    int mqid = msgget(42, IPC_CREAT | 0666);

    Msg out = {0};
    out.mtype = 1;
    strcpy(out.mtext, "hello from pid 10");
    out.msize = strlen(out.mtext) + 1;
    msgsnd(mqid, &out, out.msize, 0, &sender);

    out.mtype = 2;
    strcpy(out.mtext, "type-2 message");
    out.msize = strlen(out.mtext) + 1;
    msgsnd(mqid, &out, out.msize, 0, &sender);

    Msg in = {0};
    /* Receive type-2 first */
    msgrcv(mqid, &in, sizeof in.mtext, 2, 0, &receiver);
    printf("  received type=%ld text='%s'\n", in.mtype, in.mtext);

    /* Receive next (type 0 = first) */
    msgrcv(mqid, &in, sizeof in.mtext, 0, 0, &receiver);
    printf("  received type=%ld text='%s'\n", in.mtype, in.mtext);

    msgctl(mqid, IPC_RMID, NULL);

    /* ── 3. Shared memory ───────────────────────────────────── */
    banner("Shared Memory (shmget/shmat/shmdt/shmctl)");
    shm_init();

    int shmid = shmget(99, 1024, IPC_CREAT | 0666);

    ShmAttach attaches[MAX_SHM_ATTACHES];
    int       attach_count = 0;

    /* Process A attaches */
    void *va_a = shmat(shmid, NULL, 0, attaches, &attach_count);
    /* Process B attaches (same region) */
    void *va_b = shmat(shmid, NULL, 0, attaches, &attach_count);

    if (va_a) {
        memcpy(va_a, "shared data written by A", 25);
        printf("  A wrote: '%s'\n", (char*)va_a);
    }
    if (va_b)
        printf("  B reads: '%s'\n", (char*)va_b);

    shmdt(va_b, attaches, &attach_count);
    shmdt(va_a, attaches, &attach_count);
    shmctl(shmid, IPC_RMID, NULL);

    /* ── 4. Semaphores ──────────────────────────────────────── */
    banner("Semaphores (semget/semctl/semop)");
    sem_init_subsystem();

    int semid = semget(77, 2, IPC_CREAT | 0666);
    semctl(semid, 0, SETVAL, 1);  /* sem[0] = 1 (mutex) */
    semctl(semid, 1, SETVAL, 0);  /* sem[1] = 0 (signal)*/

    SimProcess proc_a = {.pid=20};
    SimProcess proc_b = {.pid=21};

    /* P(sem[0]): acquire mutex */
    SemBuf p_ops[1] = {{ .sem_num=0, .sem_op=-1, .sem_flg=0 }};
    semop(semid, p_ops, 1, &proc_a);

    /* V(sem[0]): release mutex */
    SemBuf v_ops[1] = {{ .sem_num=0, .sem_op=+1, .sem_flg=0 }};
    semop(semid, v_ops, 1, &proc_a);

    /* Signal sem[1] */
    SemBuf sig[1]   = {{ .sem_num=1, .sem_op=+1, .sem_flg=0 }};
    semop(semid, sig, 1, &proc_b);

    semctl(semid, 0, GETVAL, 0);
    semctl(semid, 1, GETVAL, 0);
    semctl(semid, 0, IPC_RMID, 0);

    /* ── 5. Sockets ─────────────────────────────────────────── */
    banner("Sockets (stream + datagram)");
    socket_subsystem_init();

    /* ── Stream: client/server over AF_UNIX ─────────────────── */
    printf("\n--- TCP-style stream (AF_UNIX) ---\n");
    SockAddr srv_addr = { .domain=AF_UNIX, .path="/tmp/uiox.sock" };

    /* Server */
    int srv_sd = sys_socket(AF_UNIX, SOCK_STREAM, PROTO_DEFAULT);
    sys_bind(srv_sd, &srv_addr);
    sys_listen(srv_sd, 5);

    /* Client */
    int cli_sd = sys_socket(AF_UNIX, SOCK_STREAM, PROTO_DEFAULT);
    sys_connect(cli_sd, &srv_addr);

    /* Server accepts */
    SockAddr cli_addr = {0};
    int conn_sd = sys_accept(srv_sd, &cli_addr);

    /* Client sends, server receives */
    const char *hello = "Hello, server!";
    sys_send(cli_sd, hello, strlen(hello) + 1, 0);

    char rbuf[64] = {0};
    sys_recv(conn_sd, rbuf, sizeof rbuf, 0);
    printf("  server received: '%s'\n", rbuf);

    /* Peek */
    sys_send(cli_sd, "peek-me", 8, 0);
    sys_recv(conn_sd, rbuf, sizeof rbuf, MSG_PEEK);
    sys_recv(conn_sd, rbuf, sizeof rbuf, 0);
    printf("  after peek+recv: '%s'\n", rbuf);

    /* Out-of-band */
    const char oob = '!';
    sys_send(cli_sd, &oob, 1, MSG_OOB);
    char oob_buf[2] = {0};
    sys_recv(conn_sd, oob_buf, 1, MSG_OOB);
    printf("  OOB byte: '%c'\n", oob_buf[0]);

    /* Shutdown and close */
    sys_shutdown(conn_sd, SHUT_RDWR);
    sys_close_socket(conn_sd);
    sys_close_socket(cli_sd);
    sys_close_socket(srv_sd);

    /* ── Datagram: UDP-style ─────────────────────────────────── */
    printf("\n--- UDP datagram (AF_INET) ---\n");
    SockAddr udp_srv = { .domain=AF_INET, .path="127.0.0.1:9000" };
    SockAddr udp_cli = { .domain=AF_INET, .path="127.0.0.1:0" };

    int u_srv = sys_socket(AF_INET, SOCK_DGRAM, PROTO_DEFAULT);
    int u_cli = sys_socket(AF_INET, SOCK_DGRAM, PROTO_DEFAULT);
    sys_bind(u_srv, &udp_srv);
    sys_bind(u_cli, &udp_cli);

    const char *dgram = "UDP datagram payload";
    sys_sendto(u_cli, dgram, strlen(dgram) + 1, 0, &udp_srv);

    SockAddr from = {0};
    char dbuf[64] = {0};
    sys_recvfrom(u_srv, dbuf, sizeof dbuf, 0, &from);
    printf("  server dgram received: '%s'\n", dbuf);

    SockAddr myname = {0};
    sys_getsockname(u_srv, &myname);

    sys_close_socket(u_srv);
    sys_close_socket(u_cli);

    return 0;
}
