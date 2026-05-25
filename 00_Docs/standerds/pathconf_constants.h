/**
 * @file    pathconf_constants.h
 * @brief   POSIX pathconf(2) / fpathconf(2) symbolic constant mapping.
 *
 * Maps POSIX-standard symbolic limit/option names to their PC_* codes
 * as passed to pathconf() and fpathconf(). Values are filesystem-path-
 * specific — the same name may return different values on different
 * mounted filesystems.
 *
 * Platform support matrix:
 *   PLATFORM_ARM64   — 64-bit ARM (AArch64)
 *   PLATFORM_ARM32   — 32-bit ARM
 *   PLATFORM_X86_64  — x86-64 (code 0x8064)
 *
 * Reference: POSIX.1-2017 §2.5 / The Open Group Base Specifications Issue 7
 *
 * @version 1.0.0
 * @date    2026-05-25
 */

 #ifndef PATHCONF_CONSTANTS_H
 #define PATHCONF_CONSTANTS_H
 
 #include <unistd.h>   /* pathconf(), fpathconf(), _PC_* */
 
 /* -------------------------------------------------------------------------
  * Platform identifiers
  * ---------------------------------------------------------------------- */
 
 #define PLATFORM_ARM64   "ARM64"
 #define PLATFORM_ARM32   "ARM32"
 #define PLATFORM_X86_64  0x8064   /**< x86-64 numeric platform tag */
 
 /* -------------------------------------------------------------------------
  * File-system size and name limits
  * Queried with: pathconf(path, PC_xxx) or fpathconf(fd, PC_xxx)
  * ---------------------------------------------------------------------- */
 
 /** Maximum number of bits in a file size (off_t). */
 #define PC_FILESIZEBITS     _PC_FILESIZEBITS
 
 /** Maximum number of links to a single file. */
 #define PC_LINKMAX          _PC_LINK_MAX
 
 /** Maximum number of bytes in a terminal canonical input line. */
 #define PC_MAXCANON         _PC_MAX_CANON
 
 /** Maximum number of bytes in a terminal input queue. */
 #define PC_MAXINPUT         _PC_MAX_INPUT
 
 /** Maximum length of a filename component (excluding NUL). */
 #define PC_NAMEMAX          _PC_NAME_MAX
 
 /** Maximum length of an entire pathname (excluding NUL). */
 #define PC_PATHMAX          _PC_PATH_MAX
 
 /** Size of the pipe buffer: guaranteed atomic write size in bytes. */
 #define PC_PIPEBUF          _PC_PIPE_BUF
 
 /** Maximum length of a symbolic link target string. */
 #define PC_SYMLINKMAX       _PC_SYMLINK_MAX
 
 /** Resolution of file timestamps, in nanoseconds. */
 #define PC_TIMESTAMPRESOLUTION  _PC_TIMESTAMP_RESOLUTION
 
 /* -------------------------------------------------------------------------
  * POSIX filesystem option flags
  * Return value: -1 (indeterminate), 0 (not supported), >0 (supported)
  * ---------------------------------------------------------------------- */
 
 /**
  * Non-zero if chown(2) is restricted to privileged processes.
  * Affects setuid/setgid security semantics.
  */
 #define PC_CHOWNRESTRICTED  _PC_CHOWN_RESTRICTED
 
 /**
  * Non-zero if long filenames are truncated rather than causing an error.
  * Zero means ENAMETOOLONG is returned instead.
  */
 #define PC_NOTRUNC          _PC_NO_TRUNC
 
 /**
  * Character value that disables a terminal special character.
  * Typically 0xFF or 0x00.
  */
 #define PC_VDISABLE         _PC_VDISABLE
 
 /** Non-zero if asynchronous I/O is supported on this path. */
 #define PC_ASYNCIO          _PC_ASYNC_IO
 
 /** Non-zero if prioritized I/O is supported on this path. */
 #define PC_PRIOIO           _PC_PRIO_IO
 
 /** Non-zero if synchronized I/O is supported on this path. */
 #define PC_SYNCIO           _PC_SYNC_IO
 
 /**
  * Non-zero if the filesystem supports the creation of symbolic links.
  * POSIX.1-2008 addition.
  */
 #define PC_2_SYMLINKS       _PC_2_SYMLINKS
 
 /* -------------------------------------------------------------------------
  * Convenience query helper
  * ---------------------------------------------------------------------- */
 
 #include <errno.h>
 #include <stdio.h>
 
 /**
  * @brief Query a pathconf value and print a human-readable result.
  *
  * @param path   Filesystem path to query.
  * @param name   PC_* constant (e.g. PC_NAMEMAX).
  * @param label  Human-readable label for the constant.
  */
 static inline void pathconf_print(const char *path, int name,
                                    const char *label)
 {
     errno = 0;
     long val = pathconf(path, name);
     if (val == -1) {
         if (errno)
             fprintf(stderr, "  %-30s  error: %m\n", label);
         else
             fprintf(stderr, "  %-30s  indeterminate (no limit)\n", label);
     } else {
         printf("  %-30s  %ld\n", label, val);
     }
 }
 
 #endif /* PATHCONF_CONSTANTS_H */
 