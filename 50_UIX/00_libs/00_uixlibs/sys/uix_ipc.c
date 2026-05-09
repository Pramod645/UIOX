/******************* uix_ipc.c ********************************/
#include "uix_ipc.h"
#include "uix_stat.h"
#include "uix_errno.h"

uix_key_t uix_ftok(const char *path, int id)
{
    uix_stat_t sb;
    if (uix_stat(path, &sb) < 0) return (uix_key_t)-1;
    return (uix_key_t)(
        ((uix_uint32_t)(id & 0xFF)   << 24) |
        ((uix_uint32_t)(sb.st_dev & 0xFF) << 16) |
        ((uix_uint32_t)(sb.st_ino & 0xFFFF)));
}

/* ***This is End of file, there is no more line should be added after this line*** */
