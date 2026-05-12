/*
 * main.c — Demonstrates APUE Chapter 9: Process Relationships.
 *
 * Usage: ./proc_rel [demo]
 *   demo = 1: process group demo
 *   demo = 2: session demo (Exercise 9.2)
 *   demo = 3: orphaned process group (Figure 9.12)
 *   (default): run all
 */

 #include "../include/proc_group.h"
 #include "../include/proc_session.h"
 #include "../include/proc_job.h"
 #include "../include/proc_orphan.h"
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 
 int main(int argc, char *argv[])
 {
     int demo = 0;
 
     if (argc > 1)
         demo = atoi(argv[1]);
 
     if (demo == 0 || demo == 1) {
         printf("=== Process Group Demo ===\n");
         pg_print_info("start");
         pg_demo_fork_setpgid();
         printf("\n");
     }
 
     if (demo == 0 || demo == 2) {
         printf("=== Session Demo (Exercise 9.2) ===\n");
         sess_print_relationships("before");
         sess_demo_new_session();
         printf("\n");
     }
 
     if (demo == 0 || demo == 3) {
         printf("=== Orphaned Process Group Demo (Figure 9.12) ===\n");
         printf("Note: parent exits after 5 seconds.\n");
         orphan_run_demo();
         printf("\n");
     }
 
     return 0;
 }
 