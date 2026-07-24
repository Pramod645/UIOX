#ifndef BRK_H
#define BRK_H
#include "uiox_klibc.h"

#define MIN_BRK_ADDR    0x1000
#define MAX_BRK_ADDR    0x7FFFFFFF
#define BRK_ALIGN       4096

struct proc;
struct u_area;
struct pregion;

uintptr_t kernel_brk       (uintptr_t new_brk);
uintptr_t get_current_brk  (struct u_area *u);
int       brk_check_legal  (uintptr_t new_brk, struct u_area *u);
void      brk_zero_new_space(uintptr_t old_brk, uintptr_t new_brk);
#endif /* BRK_H */
