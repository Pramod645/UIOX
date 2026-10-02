/*
 * 30_KIX/33_PCS/src/uiox_uaccess.c
 *
 * The privilege boundary — copy_to_user / copy_from_user.
 *
 *   TODO(arch): replace the two memcpy calls with the arch-specific
 *   fault-tolerant copy — `ldtr`/`sttr` on ARM64, `stac`/`clac` on
 *   x86-64, a fixup-table entry on ARM32 and RISC-V.  The prototypes
 *   already have the right shape; only the body changes.
 *
 * @version 1.0.0  @date 2026-10-03
 */
#include "uiox_uaccess.h"

extern void *memcpy(void *dst, const void *src, size_t n);

int uiox_copy_to_user(void *udst, const void *ksrc, size_t n)
{
    if (!uiox_uaccess_ok(udst, n))  return UIOX_UACCESS_EFAULT;
    if (n == 0u)                    return UIOX_UACCESS_OK;
    if (!ksrc)                      return UIOX_UACCESS_EINVAL;

    memcpy(udst, ksrc, n);
    return UIOX_UACCESS_OK;
}

int uiox_copy_from_user(void *kdst, const void *usrc, size_t n)
{
    if (!uiox_uaccess_ok(usrc, n))  return UIOX_UACCESS_EFAULT;
    if (n == 0u)                    return UIOX_UACCESS_OK;
    if (!kdst)                      return UIOX_UACCESS_EINVAL;

    memcpy(kdst, usrc, n);
    return UIOX_UACCESS_OK;
}

int uiox_put_user_u8(uint8_t val, uint8_t *uaddr)
{
    if (!uiox_uaccess_ok(uaddr, sizeof(uint8_t))) return UIOX_UACCESS_EFAULT;
    *uaddr = val;
    return UIOX_UACCESS_OK;
}

int uiox_put_user_u16(uint16_t val, uint16_t *uaddr)
{
    if (!uiox_uaccess_ok(uaddr, sizeof(uint16_t))) return UIOX_UACCESS_EFAULT;
    memcpy(uaddr, &val, sizeof(val));
    return UIOX_UACCESS_OK;
}

int uiox_put_user_u32(uint32_t val, uint32_t *uaddr)
{
    if (!uiox_uaccess_ok(uaddr, sizeof(uint32_t))) return UIOX_UACCESS_EFAULT;
    memcpy(uaddr, &val, sizeof(val));
    return UIOX_UACCESS_OK;
}

int uiox_put_user_u64(uint64_t val, uint64_t *uaddr)
{
    if (!uiox_uaccess_ok(uaddr, sizeof(uint64_t))) return UIOX_UACCESS_EFAULT;
    memcpy(uaddr, &val, sizeof(val));
    return UIOX_UACCESS_OK;
}

int uiox_get_user_u8(uint8_t *out, const uint8_t *uaddr)
{
    if (!out) return UIOX_UACCESS_EINVAL;
    if (!uiox_uaccess_ok(uaddr, sizeof(uint8_t))) return UIOX_UACCESS_EFAULT;
    *out = *uaddr;
    return UIOX_UACCESS_OK;
}

int uiox_get_user_u16(uint16_t *out, const uint16_t *uaddr)
{
    if (!out) return UIOX_UACCESS_EINVAL;
    if (!uiox_uaccess_ok(uaddr, sizeof(uint16_t))) return UIOX_UACCESS_EFAULT;
    memcpy(out, uaddr, sizeof(*out));
    return UIOX_UACCESS_OK;
}

int uiox_get_user_u32(uint32_t *out, const uint32_t *uaddr)
{
    if (!out) return UIOX_UACCESS_EINVAL;
    if (!uiox_uaccess_ok(uaddr, sizeof(uint32_t))) return UIOX_UACCESS_EFAULT;
    memcpy(out, uaddr, sizeof(*out));
    return UIOX_UACCESS_OK;
}

int uiox_get_user_u64(uint64_t *out, const uint64_t *uaddr)
{
    if (!out) return UIOX_UACCESS_EINVAL;
    if (!uiox_uaccess_ok(uaddr, sizeof(uint64_t))) return UIOX_UACCESS_EFAULT;
    memcpy(out, uaddr, sizeof(*out));
    return UIOX_UACCESS_OK;
}
