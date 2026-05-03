#include "uix_errno.h"

int uix_errno = 0;

static const char *uix_errlist[] = {
    [0]               = "Success",
    [UIX_EPERM]       = "Operation not permitted",
    [UIX_ENOENT]      = "No such file or directory",
    [UIX_ESRCH]       = "No such process",
    [UIX_EINTR]       = "Interrupted system call",
    [UIX_EIO]         = "I/O error",
    [UIX_ENXIO]       = "No such device or address",
    [UIX_E2BIG]       = "Argument list too long",
    [UIX_ENOEXEC]     = "Exec format error",
    [UIX_EBADF]       = "Bad file number",
    [UIX_ECHILD]      = "No child processes",
    [UIX_EAGAIN]      = "Try again",
    [UIX_ENOMEM]      = "Out of memory",
    [UIX_EACCES]      = "Permission denied",
    [UIX_EFAULT]      = "Bad address",
    [UIX_ENOTBLK]     = "Block device required",
    [UIX_EBUSY]       = "Device or resource busy",
    [UIX_EEXIST]      = "File exists",
    [UIX_EXDEV]       = "Cross-device link",
    [UIX_ENODEV]      = "No such device",
    [UIX_ENOTDIR]     = "Not a directory",
    [UIX_EISDIR]      = "Is a directory",
    [UIX_EINVAL]      = "Invalid argument",
    [UIX_ENFILE]      = "File table overflow",
    [UIX_EMFILE]      = "Too many open files",
    [UIX_ENOTTY]      = "Not a typewriter",
    [UIX_ETXTBSY]     = "Text file busy",
    [UIX_EFBIG]       = "File too large",
    [UIX_ENOSPC]      = "No space left on device",
    [UIX_ESPIPE]      = "Illegal seek",
    [UIX_EROFS]       = "Read-only file system",
    [UIX_EMLINK]      = "Too many links",
    [UIX_EPIPE]       = "Broken pipe",
    [UIX_EDOM]        = "Math argument out of domain",
    [UIX_ERANGE]      = "Math result not representable",
};

const char *uix_strerror(int errnum)
{
    uix_size_t sz = sizeof(uix_errlist) / sizeof(uix_errlist[0]);
    if (errnum >= 0 && (uix_size_t)errnum < sz &&
        uix_errlist[errnum])
        return uix_errlist[errnum];
    return "Unknown error";
}
