/*
 * 30_KIX/32_FS/10_scfs/src/uiox_kix_scfs_dispatch.c
 *
 * SCFS dispatch table — the bridge between syscall numbers and the fs_*()
 * bodies.  The arch trap handler calls scfs_dispatch(nr, a0..a5); this
 * resolves nr to a handler and returns its value in the return register.
 *
 * Bach Ch.7: a syscall is a trap that reads a number, indexes a table, and
 * calls a kernel routine.  The table below is that index.
 *
 * @version 1.0.0  @date 2026-09-21
 */
#include "uiox_kix_scfs_internal.h"
#include "uiox_scfs.h"      /* SCFS_NR_* numbers */

typedef long (*scfs_fn_t)(long, long, long, long, long, long);

/* Each syscall number maps to the fs_* body that implements it. */
typedef struct {
    uint32_t  nr;
    scfs_fn_t fn;
} scfs_entry_t;

/* Thunks — adapt the raw register arguments to the typed fs_* signatures. */
static long th_open  (long a,long b,long c,long d,long e,long f){(void)d;(void)e;(void)f;
    return fs_open((const char*)a,(int)b,(uint16_t)c);}
static long th_close (long a,long b,long c,long d,long e,long f){(void)b;(void)c;(void)d;(void)e;(void)f;
    return fs_close((int)a);}
static long th_read  (long a,long b,long c,long d,long e,long f){(void)d;(void)e;(void)f;
    return fs_read((int)a,(char*)b,(uint32_t)c);}
static long th_write (long a,long b,long c,long d,long e,long f){(void)d;(void)e;(void)f;
    return fs_write((int)a,(const char*)b,(uint32_t)c);}
static long th_pread (long a,long b,long c,long d,long e,long f){(void)e;(void)f;
    return fs_pread((int)a,(char*)b,(uint32_t)c,(uint32_t)d);}
static long th_pwrite(long a,long b,long c,long d,long e,long f){(void)e;(void)f;
    return fs_pwrite((int)a,(const char*)b,(uint32_t)c,(uint32_t)d);}
static long th_lseek (long a,long b,long c,long d,long e,long f){(void)d;(void)e;(void)f;
    return fs_lseek((int)a,(int32_t)b,(int)c);}
static long th_stat  (long a,long b,long c,long d,long e,long f){(void)c;(void)d;(void)e;(void)f;
    return fs_stat((const char*)a,(stat_t*)b);}
static long th_fstat (long a,long b,long c,long d,long e,long f){(void)c;(void)d;(void)e;(void)f;
    return fs_fstat((int)a,(stat_t*)b);}
static long th_lstat (long a,long b,long c,long d,long e,long f){(void)c;(void)d;(void)e;(void)f;
    return fs_lstat((const char*)a,(stat_t*)b);}
static long th_creat (long a,long b,long c,long d,long e,long f){(void)c;(void)d;(void)e;(void)f;
    return fs_creat((const char*)a,(uint16_t)b);}
static long th_mknod (long a,long b,long c,long d,long e,long f){(void)d;(void)e;(void)f;
    return fs_mknod((const char*)a,(uint16_t)b,(uint8_t)c,(uint8_t)c);}
static long th_mkdir (long a,long b,long c,long d,long e,long f){(void)c;(void)d;(void)e;(void)f;
    return fs_mkdir((const char*)a,(uint16_t)b);}
static long th_rmdir (long a,long b,long c,long d,long e,long f){(void)b;(void)c;(void)d;(void)e;(void)f;
    return fs_rmdir((const char*)a);}
static long th_chdir (long a,long b,long c,long d,long e,long f){(void)b;(void)c;(void)d;(void)e;(void)f;
    return fs_chdir((const char*)a);}
static long th_fchdir(long a,long b,long c,long d,long e,long f){(void)b;(void)c;(void)d;(void)e;(void)f;
    return fs_fchdir((int)a);}
static long th_chroot(long a,long b,long c,long d,long e,long f){(void)b;(void)c;(void)d;(void)e;(void)f;
    return fs_chroot((const char*)a);}
static long th_getcwd(long a,long b,long c,long d,long e,long f){(void)c;(void)d;(void)e;(void)f;
    return fs_getcwd((char*)a,(uint32_t)b);}
static long th_link  (long a,long b,long c,long d,long e,long f){(void)c;(void)d;(void)e;(void)f;
    return fs_link((const char*)a,(const char*)b);}
static long th_unlink(long a,long b,long c,long d,long e,long f){(void)b;(void)c;(void)d;(void)e;(void)f;
    return fs_unlink((const char*)a);}
static long th_rename(long a,long b,long c,long d,long e,long f){(void)c;(void)d;(void)e;(void)f;
    return fs_rename((const char*)a,(const char*)b);}
static long th_trunc (long a,long b,long c,long d,long e,long f){(void)c;(void)d;(void)e;(void)f;
    return fs_truncate((const char*)a,(uint64_t)b);}
static long th_ftrunc(long a,long b,long c,long d,long e,long f){(void)c;(void)d;(void)e;(void)f;
    return fs_ftruncate((int)a,(uint64_t)b);}
static long th_sync  (long a,long b,long c,long d,long e,long f){(void)a;(void)b;(void)c;(void)d;(void)e;(void)f;
    return fs_sync();}
static long th_fsync (long a,long b,long c,long d,long e,long f){(void)b;(void)c;(void)d;(void)e;(void)f;
    return fs_fsync((int)a);}
static long th_mount (long a,long b,long c,long d,long e,long f){(void)d;(void)e;(void)f;
    return fs_mount((const char*)a,(const char*)b,(int)c);}
static long th_umount(long a,long b,long c,long d,long e,long f){(void)c;(void)d;(void)e;(void)f;
    return fs_umount((const char*)a,(int)b);}
static long th_dup   (long a,long b,long c,long d,long e,long f){(void)b;(void)c;(void)d;(void)e;(void)f;
    return fs_dup((int)a);}
static long th_ioctl (long a,long b,long c,long d,long e,long f){(void)d;(void)e;(void)f;
    return fs_ioctl((int)a,(uint32_t)b,(void*)c);}

/* The table — SCFS_NR_* -> handler.  Numbers come from uiox_scfs.h. */
static const scfs_entry_t s_table[] = {
    { SCFS_NR_open,     th_open   }, { SCFS_NR_close,    th_close  },
    { SCFS_NR_read,     th_read   }, { SCFS_NR_write,    th_write  },
    { SCFS_NR_pread,    th_pread  }, { SCFS_NR_pwrite,   th_pwrite },
    { SCFS_NR_lseek,    th_lseek  },
    { SCFS_NR_stat,     th_stat   }, { SCFS_NR_fstat,    th_fstat  },
    { SCFS_NR_lstat,    th_lstat  },
    { SCFS_NR_creat,    th_creat  }, { SCFS_NR_mknod,    th_mknod  },
    { SCFS_NR_mkdir,    th_mkdir  }, { SCFS_NR_rmdir,    th_rmdir  },
    { SCFS_NR_chdir,    th_chdir  }, { SCFS_NR_fchdir,   th_fchdir },
    { SCFS_NR_chroot,   th_chroot }, { SCFS_NR_getcwd,   th_getcwd },
    { SCFS_NR_link,     th_link   }, { SCFS_NR_unlink,   th_unlink },
    { SCFS_NR_rename,   th_rename },
    { SCFS_NR_truncate, th_trunc  }, { SCFS_NR_ftruncate,th_ftrunc },
    { SCFS_NR_sync,     th_sync   }, { SCFS_NR_fsync,    th_fsync  },
    { SCFS_NR_mount,    th_mount  }, { SCFS_NR_umount,   th_umount },
    { SCFS_NR_dup,      th_dup    }, { SCFS_NR_ioctl,    th_ioctl  },
};

#define SCFS_NR_ENTRIES (sizeof(s_table)/sizeof(s_table[0]))

long scfs_dispatch(uint32_t nr, long a0, long a1, long a2,
                   long a3, long a4, long a5)
{
    for (uint32_t i = 0u; i < SCFS_NR_ENTRIES; i++)
        if (s_table[i].nr == nr)
            return s_table[i].fn(a0, a1, a2, a3, a4, a5);
    return FS_ENOSYS;
}
