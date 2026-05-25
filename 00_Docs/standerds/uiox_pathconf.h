/**
 * @file    uiox_pathconf.h
 * @brief   POSIX pathconf(2) / fpathconf(2) symbolic constants
 *
 * Maps POSIX symbolic limit names to their _PC_* query tokens used
 * with pathconf() and fpathconf() to retrieve per-path/filesystem
 * configuration limits at runtime.
 *
 * Conforms to: POSIX.1-2017 (IEEE Std 1003.1-2017 / SUSv4)
 * Target platforms: ARM64, ARM32, x86_64 (0x8064)
 *
 * Usage:
 *   long val = pathconf("/some/path", _PC_NAME_MAX);
 *
 * @author  UIOX Project
 * @date    2026-05-25
 */

 #ifndef UIOX_PATHCONF_H
 #define UIOX_PATHCONF_H
 
 #include <unistd.h>   /* pathconf(), fpathconf(), _PC_* tokens */
 
 /* ------------------------------------------------------------------ */
 /*  Platform / Architecture Selection                                  */
 /* ------------------------------------------------------------------ */
 
 #if defined(__aarch64__)
 #  define PLATFORM  "ARM64"         /**< 64-bit ARM (AArch64)         */
 #elif defined(__arm__)
 #  define PLATFORM  "ARM32"         /**< 32-bit ARM                   */
 #elif defined(__x86_64__)
 #  define PLATFORM  0x8064          /**< x86-64 (AMD64/Intel 64)      */
 #else
 #  error "Unsupported platform — add your architecture to uiox_pathconf.h"
 #endif
 
 /* ------------------------------------------------------------------ */
 /*  File / Filesystem Size & Link Limits                               */
 /* ------------------------------------------------------------------ */
 
 /**
  * @defgroup pathconf_limits  Per-path filesystem limit constants
  * @{
  *
  * Each entry pairs the POSIX symbolic name with the _PC_* token
  * accepted by pathconf(). A return value of -1 means the limit is
  * either unsupported on this filesystem or unlimited.
  */
 
 /** Minimum number of bits in file sizes and offsets.
  *  POSIX name: FILESIZEBITS  |  Query: _PC_FILESIZEBITS */
 #define UIOX_PC_FILESIZEBITS        _PC_FILESIZEBITS
 
 /** Maximum number of hard links to a single file.
  *  POSIX name: LINK_MAX       |  Query: _PC_LINK_MAX */
 #define UIOX_PC_LINKMAX             _PC_LINK_MAX
 
 /** Maximum number of bytes in a terminal canonical input queue.
  *  POSIX name: MAX_CANON      |  Query: _PC_MAX_CANON */
 #define UIOX_PC_MAXCANON            _PC_MAX_CANON
 
 /** Maximum number of bytes in a terminal raw input queue.
  *  POSIX name: MAX_INPUT      |  Query: _PC_MAX_INPUT */
 #define UIOX_PC_MAXINPUT            _PC_MAX_INPUT
 
 /** Maximum length of a filename component (excluding null terminator).
  *  POSIX name: NAME_MAX       |  Query: _PC_NAME_MAX */
 #define UIOX_PC_NAMEMAX             _PC_NAME_MAX
 
 /** Maximum length of an entire file path (including null terminator).
  *  POSIX name: PATH_MAX       |  Query: _PC_PATH_MAX */
 #define UIOX_PC_PATHMAX             _PC_PATH_MAX
 
 /** Minimum guaranteed atomic write size to a pipe, in bytes (≥512).
  *  POSIX name: PIPE_BUF       |  Query: _PC_PIPE_BUF */
 #define UIOX_PC_PIPEBUF             _PC_PIPE_BUF
 
 /** Maximum length of a symbolic link target string.
  *  POSIX name: SYMLINK_MAX    |  Query: _PC_SYMLINK_MAX */
 #define UIOX_PC_SYMLINKMAX          _PC_SYMLINK_MAX
 
 /** Resolution of file timestamps (nanoseconds).
  *  POSIX name: POSIX_TIMESTAMP_RESOLUTION  |  Query: _PC_TIMESTAMP_RESOLUTION */
 #define UIOX_PC_TIMESTAMPRESOLUTION _PC_TIMESTAMP_RESOLUTION
 
 /* ------------------------------------------------------------------ */
 /*  Filesystem Behavior Options                                        */
 /* ------------------------------------------------------------------ */
 
 /** Non-zero if chown(2) is restricted to privileged processes only.
  *  POSIX name: _POSIX_CHOWN_RESTRICTED  |  Query: _PC_CHOWN_RESTRICTED */
 #define UIOX_PC_CHOWNRESTRICTED     _PC_CHOWN_RESTRICTED
 
 /** Non-zero if filenames longer than NAME_MAX generate an error
  *  instead of silent truncation.
  *  POSIX name: _POSIX_NO_TRUNC  |  Query: _PC_NO_TRUNC */
 #define UIOX_PC_NOTRUNC             _PC_NO_TRUNC
 
 /** Value of the character used to disable special terminal characters.
  *  POSIX name: _POSIX_VDISABLE  |  Query: _PC_VDISABLE */
 #define UIOX_PC_VDISABLE            _PC_VDISABLE
 
 /** Non-zero if asynchronous I/O is supported for this file.
  *  POSIX name: _POSIX_ASYNC_IO  |  Query: _PC_ASYNC_IO */
 #define UIOX_PC_ASYNCIO             _PC_ASYNC_IO
 
 /** Non-zero if prioritized asynchronous I/O is supported for this file.
  *  POSIX name: _POSIX_PRIO_IO   |  Query: _PC_PRIO_IO */
 #define UIOX_PC_PRIOIO              _PC_PRIO_IO
 
 /** Non-zero if synchronized I/O is supported for this file.
  *  POSIX name: _POSIX_SYNC_IO   |  Query: _PC_SYNC_IO */
 #define UIOX_PC_SYNCIO              _PC_SYNC_IO
 
 /** Non-zero if the filesystem supports the creation of symbolic links.
  *  POSIX name: _POSIX2_SYMLINKS  |  Query: _PC_2_SYMLINKS */
 #define UIOX_PC_2SYMLINKS           _PC_2_SYMLINKS
 
 /** @} */  /* end pathconf_limits */
 
 /* ------------------------------------------------------------------ */
 /*  Convenience query macro                                            */
 /* ------------------------------------------------------------------ */
 
 /**
  * @brief   Query a per-path configuration limit.
  * @param   _path   Filesystem path to query against.
  * @param   _token  One of the UIOX_PC_* tokens defined above.
  * @return  Long integer limit value, or -1 if unsupported/unlimited.
  *
  * Example:
  *   long max_name = UIOX_PATHCONF("/var", UIOX_PC_NAMEMAX);
  */
 #define UIOX_PATHCONF(_path, _token)  pathconf((_path), (_token))
 
 #endif /* UIOX_PATHCONF_H */
 