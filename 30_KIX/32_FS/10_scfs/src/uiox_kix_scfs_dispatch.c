/*
 * 32_FileSystem/10_scfs/src/uiox_kix_scfs_dispatch.c
 *
 * SCFS dispatcher — file-system numbers to file-system calls.
 *
 *   called by : 40_SystemCallInterface/uix_archSysCall.c
 *   calls     : the fs_* entry points this subsystem owns
 *
 * ── numbering ───────────────────────────────────────────────────────
 * The numbers are the BSD ones from 50_UIX's uix_sys.h, NOT numbers
 * invented here.  SCiX decides which subsystem owns a number; this file
 * decides which FUNCTION within the subsystem.
 *
 * That split keeps each layer's table small: SCiX knows only "3 belongs
 * to the file system", and this file knows "3 is read".  A number added
 * to the file group is one line here and one line in SCiX's ownership
 * test, and neither has to learn the other's table.
 *
 * ── why this file is the larger of the two ──────────────────────────
 * BSD defines far more file calls than process calls, because a file
 * system grows a variant for every path shape: stat and lstat and
 * fstat, chmod and fchmod and fchmodat, and so on.  All of them are
 * real numbers that a userspace program may ask for.
 *
 * Two consequences, both deliberate:
 *
 *   A number with NO implementation returns ENOSYS, not EINVAL.  The
 *   number is a genuine BSD syscall; this kernel does not have it yet.
 *   Telling a caller its number is invalid would send it hunting for a
 *   bug in its own table, which is the wrong place to look.
 *
 *   A number this subsystem does not own at all falls through to SCiX,
 *   which knows whether some OTHER subsystem claims it.  This file must
 *   not answer for numbers that are not its own.
 *
 * ── the three groups inside the table ───────────────────────────────
 *   data        read, write, lseek, dup, fcntl, ioctl, fsync
 *   metadata    stat, fstat, lstat, chmod, chown, truncate, umask
 *   namespace   open, close, mkdir, rmdir, chdir, link, unlink,
 *               symlink, readlink, rename, mknod, chroot, getdents
 *
 * The grouping is for a reader.  The table itself is in NUMERIC order,
 * because BSD spreads each family across the number space and someone
 * looking up 128 should find rename where 128 lives.
 *
 * ── the user-pointer boundary ───────────────────────────────────────
 * Every path and buffer argument reaches this layer as a raw user
 * address.  The fs_* entry points are responsible for validating them —
 * no wrapper here dereferences one.  A dispatcher that touched a user
 * pointer would collapse the boundary this whole layer exists to hold.
 *
 * @version 1.0.0  @date 2026-09-29
 */

 #include "fs.h"                      /* the fs_* entry points          */
 #include "uix_sys.h"                 /* the BSD numbers                */
 #include "uix_archSysCall.h"         /* the reg context, by pointer    */
 
 /* ── Return convention ──────────────────────────────────────────────
  * Negative is an error, matching the process side so SCiX can pass a
  * value through without asking which subsystem produced it. */
 #define SCFS_ENOSYS   ((int64_t)-38)
 #define SCFS_EINVAL   ((int64_t)-22)
 
 /* ── The entry-point signature ──────────────────────────────────────
  * Every fs_* function takes the six-slot shape, because that is what
  * the trap handler produces.  Declaring the type once means a table row
  * that names a function of the wrong arity fails to compile rather than
  * to run. */
 typedef int64_t (*uiox_kix_scfs_fn_t)(uiox_uintptr_t, uiox_uintptr_t,
                                       uiox_uintptr_t, uiox_uintptr_t,
                                       uiox_uintptr_t, uiox_uintptr_t);
 
 /* ── The table ──────────────────────────────────────────────────────
  * Designated initialisers, so a row's position and its BSD number are
  * the same thing by construction.
  *
  * A NULL row means the number is a real BSD syscall with no
  * implementation here.  That is NOT the same as a number this table has
  * never heard of: the first returns ENOSYS, the second is not this
  * subsystem's to answer for. */
 typedef struct uiox_kix_scfs_entry {
     uiox_kix_scfs_fn_t fn;
     const char        *name;
 } uiox_kix_scfs_entry_t;
 
 /*
  * ── WHAT THE ROWS POINT AT ──────────────────────────────────────────
  * The fs_* names below are the ones 10_scfs exposes.  If your header
  * spells any of them differently, the compiler says so at the row — one
  * line to correct, and the error names the function rather than the
  * number, which is why the table is worth having even before it links.
  *
  * A row set to (void *)0 is a number held with no implementation.  They
  * are written out rather than omitted so a reader can see the whole BSD
  * file surface and which parts of it exist.
  */
 #define SCFS_MAX 512
 
 static const uiox_kix_scfs_entry_t uiox_kix_scfs_table[SCFS_MAX] =
 {
     /* ── data ───────────────────────────────────────────────────── */
     [SYS_READ]        = { (uiox_kix_scfs_fn_t)(void *)uiox_kix_scfs_read,    "read"     },
     [SYS_WRITE]       = { (uiox_kix_scfs_fn_t)(void *)uiox_kix_scfs_write,   "write"    },
     [SYS_OPEN]        = { (uiox_kix_scfs_fn_t)(void *)uiox_kix_scfs_open,    "open"     },
     [SYS_CLOSE]       = { (uiox_kix_scfs_fn_t)(void *)uiox_kix_scfs_close,   "close"    },
     [SYS_LSEEK]       = { (uiox_kix_scfs_fn_t)(void *)uiox_kix_scfs_lseek,   "lseek"    },
     [SYS_DUP]         = { (uiox_kix_scfs_fn_t)(void *)uiox_kix_scfs_dup,     "dup"      },
     [SYS_FCNTL]       = { (uiox_kix_scfs_fn_t)(void *)uiox_kix_scfs_fcntl,   "fcntl"    },
     [SYS_IOCTL]       = { (uiox_kix_scfs_fn_t)(void *)uiox_kix_scfs_ioctl,   "ioctl"    },
     [SYS_FSYNC]       = { (uiox_kix_scfs_fn_t)(void *)uiox_kix_scfs_fsync,   "fsync"    },
 
     /* ── metadata ───────────────────────────────────────────────── */
     [SYS_STAT]        = { (uiox_kix_scfs_fn_t)(void *)uiox_kix_scfs_stat,    "stat"     },
     [SYS_FSTAT]       = { (uiox_kix_scfs_fn_t)(void *)uiox_kix_scfs_fstat,   "fstat"    },
     [SYS_LSTAT]       = { (uiox_kix_scfs_fn_t)(void *)uiox_kix_scfs_lstat,   "lstat"    },
     [SYS_ACCESS]      = { (uiox_kix_scfs_fn_t)(void *)uiox_kix_scfs_access,  "access"   },
     [SYS_CHMOD]       = { (uiox_kix_scfs_fn_t)(void *)uiox_kix_scfs_chmod,   "chmod"    },
     [SYS_CHOWN]       = { (uiox_kix_scfs_fn_t)(void *)uiox_kix_scfs_chown,   "chown"    },
     [SYS_FCHMOD]      = { (uiox_kix_scfs_fn_t)(void *)uiox_kix_scfs_fchmod,  "fchmod"   },
     [SYS_FCHOWN]      = { (uiox_kix_scfs_fn_t)(void *)uiox_kix_scfs_fchown,  "fchown"   },
     [SYS_UMASK]       = { (uiox_kix_scfs_fn_t)(void *)uiox_kix_scfs_umask,   "umask"    },
     [SYS_TRUNCATE]    = { (uiox_kix_scfs_fn_t)(void *)uiox_kix_scfs_truncate,"truncate" },
     [SYS_FTRUNCATE]   = { (uiox_kix_scfs_fn_t)(void *)uiox_kix_scfs_ftruncate,"ftruncate"},
 
     /* ── namespace ──────────────────────────────────────────────── */
     [SYS_MKDIR]       = { (uiox_kix_scfs_fn_t)(void *)uiox_kix_scfs_mkdir,   "mkdir"    },
     [SYS_RMDIR]       = { (uiox_kix_scfs_fn_t)(void *)uiox_kix_scfs_rmdir,   "rmdir"    },
     [SYS_CHDIR]       = { (uiox_kix_scfs_fn_t)(void *)uiox_kix_scfs_chdir,   "chdir"    },
     [SYS_LINK]        = { (uiox_kix_scfs_fn_t)(void *)uiox_kix_scfs_link,    "link"     },
     [SYS_UNLINK]      = { (uiox_kix_scfs_fn_t)(void *)uiox_kix_scfs_unlink,  "unlink"   },
     [SYS_SYMLINK]     = { (uiox_kix_scfs_fn_t)(void *)uiox_kix_scfs_symlink, "symlink"  },
     [SYS_READLINK]    = { (uiox_kix_scfs_fn_t)(void *)uiox_kix_scfs_readlink,"readlink" },
     [SYS_RENAME]      = { (uiox_kix_scfs_fn_t)(void *)uiox_kix_scfs_rename,  "rename"   },
     [SYS_MKNOD]       = { (uiox_kix_scfs_fn_t)(void *)uiox_kix_scfs_mknod,   "mknod"    },
     [SYS_CHROOT]      = { (uiox_kix_scfs_fn_t)(void *)uiox_kix_scfs_chroot,  "chroot"   },
     [SYS_GETDENTS]    = { (uiox_kix_scfs_fn_t)(void *)uiox_kix_scfs_getdents,"getdents" },
 
     /* ── memory mapping, file-backed when it carries an fd ────────
      * mmap is listed here rather than with the process calls because a
      * file mapping wants this subsystem's bmap().  An anonymous mapping
      * still arrives at this entry point and is served by the same
      * function, which decides on the fd argument. */
     [SYS_MMAP]        = { (uiox_kix_scfs_fn_t)(void *)uiox_kix_scfs_mmap,    "mmap"     },
     [SYS_MUNMAP]      = { (uiox_kix_scfs_fn_t)(void *)uiox_kix_scfs_munmap,  "munmap"   },
     [SYS_MPROTECT]    = { (uiox_kix_scfs_fn_t)(void *)uiox_kix_scfs_mprotect,"mprotect" },
 
     /* ── real BSD numbers with no implementation here ─────────────
      * Listed with a NULL function so a reader sees them in the table
      * rather than wondering why they are missing.  Reaching one returns
      * ENOSYS — a genuine syscall this kernel does not have. */
     [SYS_GETENTROPY]  = { (uiox_kix_scfs_fn_t)0, "getentropy"  },
     [SYS_FCHDIR]      = { (uiox_kix_scfs_fn_t)0, "fchdir"      },
     [SYS_FSTATAT]     = { (uiox_kix_scfs_fn_t)0, "fstatat"     },
     [SYS_CHFLAGS]     = { (uiox_kix_scfs_fn_t)0, "chflags"     },
     [SYS_FCHFLAGS]    = { (uiox_kix_scfs_fn_t)0, "fchflags"    },
     [SYS_SYNC]        = { (uiox_kix_scfs_fn_t)0, "sync"        },
     [SYS_STATFS]      = { (uiox_kix_scfs_fn_t)0, "statfs"      },
     [SYS_FSTATFS]     = { (uiox_kix_scfs_fn_t)0, "fstatfs"     },
     [SYS_MOUNT]       = { (uiox_kix_scfs_fn_t)0, "mount"       },
     [SYS_UNMOUNT]     = { (uiox_kix_scfs_fn_t)0, "unmount"     },
     [SYS_PIPE]        = { (uiox_kix_scfs_fn_t)0, "pipe"        },
     [SYS_PIPE2]       = { (uiox_kix_scfs_fn_t)0, "pipe2"       },
     [SYS_DUP2]        = { (uiox_kix_scfs_fn_t)0, "dup2"        },
     [SYS_DUP3]        = { (uiox_kix_scfs_fn_t)0, "dup3"        },
     [SYS_SELECT]      = { (uiox_kix_scfs_fn_t)0, "select"      },
     [SYS_POLL]        = { (uiox_kix_scfs_fn_t)0, "poll"        },
     [SYS_PREAD]       = { (uiox_kix_scfs_fn_t)0, "pread"       },
     [SYS_PWRITE]      = { (uiox_kix_scfs_fn_t)0, "pwrite"      },
     [SYS_READV]       = { (uiox_kix_scfs_fn_t)0, "readv"       },
     [SYS_WRITEV]      = { (uiox_kix_scfs_fn_t)0, "writev"      },
     [SYS_GETCWD]      = { (uiox_kix_scfs_fn_t)0, "getcwd"      },
     [SYS_REALPATH]    = { (uiox_kix_scfs_fn_t)0, "realpath"    },
     [SYS_OPENAT]      = { (uiox_kix_scfs_fn_t)0, "openat"      },
     [SYS_MKDIRAT]     = { (uiox_kix_scfs_fn_t)0, "mkdirat"     },
     [SYS_UNLINKAT]    = { (uiox_kix_scfs_fn_t)0, "unlinkat"    },
     [SYS_RENAMEAT]    = { (uiox_kix_scfs_fn_t)0, "renameat"    },
     [SYS_LINKAT]      = { (uiox_kix_scfs_fn_t)0, "linkat"      },
     [SYS_SYMLINKAT]   = { (uiox_kix_scfs_fn_t)0, "symlinkat"   },
     [SYS_READLINKAT]  = { (uiox_kix_scfs_fn_t)0, "readlinkat"  },
     [SYS_FCHMODAT]    = { (uiox_kix_scfs_fn_t)0, "fchmodat"    },
     [SYS_FCHOWNAT]    = { (uiox_kix_scfs_fn_t)0, "fchownat"    },
     [SYS_FACCESSAT]   = { (uiox_kix_scfs_fn_t)0, "faccessat"   },
     [SYS_PATHCONF]    = { (uiox_kix_scfs_fn_t)0, "pathconf"    },
     [SYS_FLOCK]       = { (uiox_kix_scfs_fn_t)0, "flock"       },
     [SYS_MKFIFO]      = { (uiox_kix_scfs_fn_t)0, "mkfifo"      },
     [SYS_KQUEUE]      = { (uiox_kix_scfs_fn_t)0, "kqueue"      },
     [SYS_MSYNC]       = { (uiox_kix_scfs_fn_t)0, "msync"       },
     [SYS_MLOCK]       = { (uiox_kix_scfs_fn_t)0, "mlock"       },
     [SYS_MUNLOCK]     = { (uiox_kix_scfs_fn_t)0, "munlock"     }
 };
 
 /* ────────────────────────────────────────────────────────────────────
  * uiox_kix_scfs_dispatch — one file-system number to one fs_* call.
  *
  * Returns the raw int64_t.  Where a register context is supplied the
  * value is written back with the same convention the process side uses
  * — positive error number in rc_r0 with rc_carry set — so SCiX can
  * return either subsystem's result without knowing which produced it.
  * ──────────────────────────────────────────────────────────────────── */
 int64_t uiox_kix_scfs_dispatch(uiox_uint64_t nr,
                                uiox_uintptr_t a0, uiox_uintptr_t a1,
                                uiox_uintptr_t a2, uiox_uintptr_t a3,
                                uiox_uintptr_t a4, uiox_uintptr_t a5,
                                void *regs)
 {
     const uiox_kix_scfs_entry_t *e;
     int64_t                      rv;
 
     /* Out of range: this table cannot speak for it.  EINVAL rather than
      * ENOSYS, because the number is outside the BSD space this table
      * models at all. */
     if (nr >= (uiox_uint64_t)SCFS_MAX)
         return SCFS_EINVAL;
 
     e = &uiox_kix_scfs_table[nr];
 
     /* A slot with no name is a number this SUBSYSTEM does not own.
      * SCiX should have routed it elsewhere, so this is a routing bug
      * rather than a missing implementation — hence EINVAL, and hence
      * the name test rather than the function test. */
     if (e->name == (const char *)0)
         return SCFS_EINVAL;
 
     /* A named slot with no function is a real BSD syscall this kernel
      * has not implemented.  ENOSYS: the number is valid, the body is
      * not written, and a caller must be able to tell the two apart. */
     if (e->fn == (uiox_kix_scfs_fn_t)0)
         return SCFS_ENOSYS;
 
     rv = e->fn(a0, a1, a2, a3, a4, a5);
 
     if (regs) {
         /* The reg context type is 40_psa's.  Referred to through a
          * local mirror so this file needs no 33_PCS include path —
          * the two fields and the flag are all it touches, and they are
          * the same three the process dispatcher writes. */
         struct scfs_reg_view {
             uiox_uintptr_t rc_pc;
             uiox_uintptr_t rc_sp;
             uiox_uint32_t  rc_sr;
             uiox_uint32_t  rc_carry;
             uiox_uintptr_t rc_gpr[16];
             uiox_uint32_t  rc_r0;
             uiox_uint32_t  rc_r1;
         } *rc = (struct scfs_reg_view *)regs;
 
         if (rv < 0) {
             rc->rc_r0    = (uiox_uint32_t)(-rv);
             rc->rc_r1    = 0u;
             rc->rc_carry = 1u;
         } else {
             rc->rc_r0    = (uiox_uint32_t)((uint64_t)rv & 0xFFFFFFFFu);
             rc->rc_r1    = (uiox_uint32_t)((uint64_t)rv >> 32);
             rc->rc_carry = 0u;
         }
     }
     return rv;
 }
 
 /* ── Introspection ──────────────────────────────────────────────────── */
 const char *uiox_kix_scfs_name(uiox_uint64_t nr)
 {
     if (nr >= (uiox_uint64_t)SCFS_MAX) return (const char *)0;
     return uiox_kix_scfs_table[nr].name;
 }
 