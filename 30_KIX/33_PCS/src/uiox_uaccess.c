/*
 * 30_KIX/33_PCS/src/uiox_uaccess.c
 *
 * User-space memory access implementation.
 *
 * Uses plain memcpy after uiox_uaccess_ok validation.
 * In a full MMU kernel this would use arch-specific
 * fault-tolerant copy (ldtr/sttr on ARM64, stac/clac on x86).
 * For UIOX at this stage, validation + memcpy is correct and safe
 * because the MMU is configured to fault on invalid user addresses.
 *
 * @version 1.0.0
 * @date    2026-07-28
 */

#include "uiox_uaccess.h"
#include "uiox_soc_stdio.h" /* memcpy */

/* ── copy_to_user ──────────────────────────────────────────────────── */
int uiox_copy_to_user(void *udst, const void *ksrc, uiox_size_t n)
{
    if (!uiox_uaccess_ok(udst, n))
        return UIOX_UACCESS_EFAULT;
    if (n == 0u)
        return UIOX_UACCESS_OK;
    if (!ksrc)
        return UIOX_UACCESS_EINVAL;

    memcpy(udst, ksrc, n);
    return UIOX_UACCESS_OK;
}

/* ── copy_from_user ────────────────────────────────────────────────── */
int uiox_copy_from_user(void *kdst, const void *usrc, uiox_size_t n)
{
    if (!uiox_uaccess_ok(usrc, n))
        return UIOX_UACCESS_EFAULT;
    if (n == 0u)
        return UIOX_UACCESS_OK;
    if (!kdst)
        return UIOX_UACCESS_EINVAL;

    memcpy(kdst, usrc, n);
    return UIOX_UACCESS_OK;
}

/* ── put_user helpers ──────────────────────────────────────────────── */
int uiox_put_user_u8(uiox_uint8_t val, uiox_uint8_t *uaddr)
{
    if (!uiox_uaccess_ok(uaddr, sizeof(uiox_uint8_t)))
        return UIOX_UACCESS_EFAULT;
    *uaddr = val;
    return UIOX_UACCESS_OK;
}

int uiox_put_user_u16(uiox_uint16_t val, uiox_uint16_t *uaddr)
{
    if (!uiox_uaccess_ok(uaddr, sizeof(uiox_uint16_t)))
        return UIOX_UACCESS_EFAULT;
    memcpy(uaddr, &val, sizeof(val));
    return UIOX_UACCESS_OK;
}

int uiox_put_user_u32(uiox_uint32_t val, uiox_uint32_t *uaddr)
{
    if (!uiox_uaccess_ok(uaddr, sizeof(uiox_uint32_t)))
        return UIOX_UACCESS_EFAULT;
    memcpy(uaddr, &val, sizeof(val));
    return UIOX_UACCESS_OK;
}

int uiox_put_user_u64(uiox_uint64_t val, uiox_uint64_t *uaddr)
{
    if (!uiox_uaccess_ok(uaddr, sizeof(uiox_uint64_t)))
        return UIOX_UACCESS_EFAULT;
    memcpy(uaddr, &val, sizeof(val));
    return UIOX_UACCESS_OK;
}

/* ── get_user helpers ──────────────────────────────────────────────── */
int uiox_get_user_u8(uiox_uint8_t *out, const uiox_uint8_t *uaddr)
{
    if (!uiox_uaccess_ok(uaddr, sizeof(uiox_uint8_t)))
        return UIOX_UACCESS_EFAULT;
    *out = *uaddr;
    return UIOX_UACCESS_OK;
}

int uiox_get_user_u16(uiox_uint16_t *out, const uiox_uint16_t *uaddr)
{
    if (!uiox_uaccess_ok(uaddr, sizeof(uiox_uint16_t)))
        return UIOX_UACCESS_EFAULT;
    memcpy(out, uaddr, sizeof(*out));
    return UIOX_UACCESS_OK;
}

int uiox_get_user_u32(uiox_uint32_t *out, const uiox_uint32_t *uaddr)
{
    if (!uiox_uaccess_ok(uaddr, sizeof(uiox_uint32_t)))
        return UIOX_UACCESS_EFAULT;
    memcpy(out, uaddr, sizeof(*out));
    return UIOX_UACCESS_OK;
}

int uiox_get_user_u64(uiox_uint64_t *out, const uiox_uint64_t *uaddr)
{
    if (!uiox_uaccess_ok(uaddr, sizeof(uiox_uint64_t)))
        return UIOX_UACCESS_EFAULT;
    memcpy(out, uaddr, sizeof(*out));
    return UIOX_UACCESS_OK;
}
