/**
 * @file  uiox_fw_rc.h
 * @brief Translate 02_FwHal's uiox_fw_err_t into negative errno.
 *
 * PATH  30_KIX/34_DSS/20_DriverInterfaces/include/uiox_fw_rc.h
 *
 * The two error spaces overlap numerically: UIOX_FW_ERR_TIMEOUT is -4,
 * which is also -EINTR; UIOX_FW_ERR_IO is -6, which is also -ENXIO. A cast
 * would silently misreport, so the mapping is explicit.
 *
 * UIOX_FW_ERR_UNSUP is a macro alias for UIOX_FW_ERR_NOT_SUPPORTED and
 * must NOT get its own case — that is a duplicate-case error.
 */
#ifndef UIOX_FW_RC_H
#define UIOX_FW_RC_H

#include "uiox_fw_types.h"
#include "uiox_klibc.h"

static inline int uiox_fw_rc(uiox_fw_err_t e)
{
    switch (e) {
    case UIOX_FW_OK:                return  0;
    case UIOX_FW_ERR_GENERIC:       return -EIO;
    case UIOX_FW_ERR_INVAL:         return -EINVAL;
    case UIOX_FW_ERR_NOMEM:         return -ENOMEM;
    case UIOX_FW_ERR_TIMEOUT:       return -ETIMEDOUT;
    case UIOX_FW_ERR_BUSY:          return -EBUSY;
    case UIOX_FW_ERR_IO:            return -EIO;
    case UIOX_FW_ERR_NODEV:         return -ENODEV;
    case UIOX_FW_ERR_NOINIT:        return -ENODEV;
    case UIOX_FW_ERR_OVERFLOW:      return -EOVERFLOW;
    case UIOX_FW_ERR_UNDERFLOW:     return -ERANGE;
    case UIOX_FW_ERR_NOT_SUPPORTED: return -ENOSYS;
    case UIOX_FW_ERR_PERM:          return -EPERM;
    case UIOX_FW_ERR_NACK:          return -EREMOTEIO;
    case UIOX_FW_ERR_ARB_LOST:      return -EAGAIN;
    case UIOX_FW_ERR_CRC:           return -EBADMSG;
    case UIOX_FW_ERR_BADMAGIC:      return -EBADMSG;
    default:                        return -EIO;
    }
}

#endif /* UIOX_FW_RC_H */
