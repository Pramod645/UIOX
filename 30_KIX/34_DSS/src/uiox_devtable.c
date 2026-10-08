/*
 *  30_KIX/34_DSS/src/uiox_devtable.c
 *
 *  The binding registry: (cls, unit) -> uiox_dev_dev_t *.
 *
 *  Why it is not in the filesystem: 32_FS must not know which classes
 *  exist.  It holds a class value and an instance number, hands both here,
 *  and gets a descriptor or nothing.  Adding a device class never touches
 *  32_FS — which is the whole point of the class numbering in
 *  uiox_devclass.h.
 *
 *  Why a scan and not a hash: the key is dense, the table is short (at
 *  most UIOX_DEV_MAX), and bindings are written once at probe and read
 *  on the ioctl path.  A hash would add a function and a collision case
 *  to save a handful of compares on a call that is already going to a
 *  bus transaction.
 *
 *  Threading: there is no lock.  bind() runs during probe, before the
 *  scheduler starts anything that can call ioctl; lookup() runs after.
 *  If a driver ever binds at runtime this needs a reader-writer lock,
 *  and that is the comment that will be wrong first.
 *
 *  v1.0.0  @date 2026-10-07
 */
#include "uiox_devclass.h"

/* Pointer slots, not copies — see the note on bind() in the header. */
static const uiox_dev_dev_t *s_dev_slot[UIOX_DEV_MAX];
static uint32_t               s_dev_count;

int uiox_dev_bind(const uiox_dev_dev_t *dev)
{
    if (!dev) return -EINVAL;

    /* A device with no class cannot be looked up, so binding it would
     * only consume a slot a real device could use. */
    if (dev->cls == UIOX_DEVCLASS_NONE) return -EINVAL;

    for (uint32_t i = 0u; i < s_dev_count; i++) {
        if (s_dev_slot[i]->cls  == dev->cls &&
            s_dev_slot[i]->unit == dev->unit)
            return -EEXIST;             /* one binding per (cls, unit) */
    }

    if (s_dev_count >= UIOX_DEV_MAX) return -ENOSPC;

    s_dev_slot[s_dev_count++] = dev;
    return 0;
}

uiox_dev_dev_t *uiox_dev_lookup(uiox_devclass_t cls, uint16_t unit)
{
    /* UIOX_DEVCLASS_NONE is the sentinel an unset inode field holds.
     * Returning NULL for it keeps the caller from having to special-case
     * the "special file that names no device" case separately. */
    if (cls == UIOX_DEVCLASS_NONE) return (uiox_dev_dev_t *)0;

    for (uint32_t i = 0u; i < s_dev_count; i++) {
        if (s_dev_slot[i]->cls  == cls &&
            s_dev_slot[i]->unit == unit)
            return (uiox_dev_dev_t *)s_dev_slot[i];
    }
    return (uiox_dev_dev_t *)0;
}

void uiox_dev_unbind_all(void)
{
    for (uint32_t i = 0u; i < s_dev_count; i++)
        s_dev_slot[i] = (const uiox_dev_dev_t *)0;
    s_dev_count = 0u;
}
