/*
 * 30_KIX/30_DSS/src/uiox_dss_init.c
 *
 * Bach's switch table, populated.
 *
 * ── what this file is, in Bach's terms ──────────────────────────────
 * Bach Ch.10, both the read and the ioctl algorithms, reach the driver
 * through a numbered entry in a per-subsystem table:
 *
 *     "the kernel routes the call through the device's entry in the
 *      character or block device switch table, and the DEVICE DRIVER
 *      performs the operation."
 *
 * In UNIX that table is bdevsw[]/cdevsw[], indexed by a MAJOR NUMBER —
 * a scarce 8-bit resource somebody has to allocate and keep unique.
 * UIOX has no such problem: uiox_devclass_t packs the family and the
 * class into one 16-bit value, so (cls, unit) is the key and nothing is
 * allocated by anyone.  uiox_devtable.c is the table; this file is the
 * one thing that puts entries in it.
 *
 * ── why it must exist ───────────────────────────────────────────────
 * Without a caller here, the table is EMPTY and step 3 of both
 * algorithms answers SCFS_ENODEV for every device node, whatever else
 * is correct.  That was the state before this file was written.
 *
 * ── the ownership rule that shapes every probe ──────────────────────
 * uiox_dev_bind() stores the POINTER it is given, not a copy
 * (uiox_devclass.h SECTION 9).  So each class's uiox_dev_dev_t must be
 * static and must outlive this call — which is why every class's probe
 * holds its own file-static descriptor, and why this file never builds
 * one on the stack.
 *
 * ── what this file deliberately does NOT do ─────────────────────────
 * It does not include a driver's headers, and it does not know a
 * class's internals.  Each probe is declared by its own class and does
 * its own bring-up; this file only ORDERS the calls, in family order.
 *
 * It also does not depend on 34_HM.  A class whose pool lives there
 * reaches it through its own translation units; the switch table has no
 * opinion about pools.
 *
 * @version 1.0.0  @date 2026-10-15
 */
#include "uiox_devclass.h"

/* ═════════════════════════════════════════════════════════════════════
 * The probes
 *
 * One per class.  Each is declared where its class lives and does three
 * things: bring up the class's subsystem, bind a static descriptor, and
 * return 0 or a negative errno.  A failure does NOT stop the others — a
 * machine with no camera must still find its keyboard.
 *
 * Add a class by adding a declaration and a call.  Nothing else in this
 * file changes, which is the point: the switch table is one list.
 * ═════════════════════════════════════════════════════════════════════ */

/* ── family 1 — BLOCK ───────────────────────────────────────────────
 * Data path: 00_bcache, keyed (dev, blkno).  The driver receives a
 * buffer from bcache_plat_read_block and never allocates one.
 * Lane 2: uiox_emmc_subsys (init sequence, EXT_CSD health). */
int uiox_emmc_probe(void);
/* ── family 2 — STREAM ────────────────────────────────────────
     * eth first: its interfaces are the fully-read ones, and the other
     * six follow the same shape. */
int uiox_eth_probe(void);        /* ◀ family 2 — STREAM */
int uiox_wifi_probe(void);       /* ◀ family 2 — STREAM */
int uiox_bt_probe(void);
int uiox_usb_probe(void);          /* prototype, after uiox_bt_probe's */

/* ── families 2–6 — not yet written ─────────────────────────────────
 * Each follows the shape of uiox_emmc_probe(): a static subsystem, a
 * static uiox_dev_dev_t, uiox_dev_bind(), and the class's own
 * dev->ops->ioctl.  Listed here as the classes land:
 *
 *   family 2 STREAM  bt  eth  wifi  usb  mic  speaker  monitor
 *   family 3 EVENT   kbd  mouse  touchpwd
 *   family 4 REG     als  thermal  bms  chg  pmic  rtc  fan
 *   family 5 CMD     gpu  hdmi  camera
 *   family 6 BUS     tb4  usb(host)
 * ──────────────────────────────────────────────────────────────────── */

/* ═════════════════════════════════════════════════════════════════════
 * uiox_dss_init — fill Bach's table
 *
 * Called once, at boot, before any device node can be opened.  Order
 * among the probes does not matter for correctness — the key is
 * (cls, unit), not a position — but keeping family order here makes the
 * list readable and matches uiox_devparse.c's table order.
 *
 * Returns 0 when the table has been populated.  A class that fails to
 * probe is reported by its own return value and skipped; the return of
 * this function is 0 as long as the TABLE was walked, because "no
 * camera present" is not an initialisation failure.
 * ═════════════════════════════════════════════════════════════════════ */
int uiox_dss_init(void)
{
    /* ── family 1 — BLOCK ───────────────────────────────────────── */
    (void)uiox_emmc_probe();
    /* ── family 2 — STREAM ────────────────────────────────────────
     * eth first: its interfaces are the fully-read ones, and the other
     * six follow the same shape. */
    (void)uiox_eth_probe();
    (void)uiox_wifi_probe(); 
    (void)uiox_bt_probe();
    (void)uiox_usb_probe();        /* live call, after (void)uiox_bt_probe(); */



    /* ── the remaining five families ─────────────────────────────
     * One call each, as their probes are written:
     *
     *   (void)uiox_bt_probe();        (void)uiox_eth_probe();
     *   (void)uiox_wifi_probe();      (void)uiox_usb_probe();
     *   (void)uiox_mic_probe();       (void)uiox_spk_probe();
     *
     *   (void)uiox_kbd_probe();       (void)uiox_mouse_probe();
     *   (void)uiox_tpwd_probe();
     *
     *   (void)uiox_als_probe();       (void)uiox_thermal_probe();
     *   (void)uiox_bms_probe();       (void)uiox_chg_probe();
     *   (void)uiox_pmic_probe();      (void)uiox_rtc_probe();
     *   (void)uiox_fan_probe();
     *
     *   (void)uiox_gpu_probe();       (void)uiox_hdmi_probe();
     *   (void)uiox_mon_probe();       (void)uiox_cam_probe();
     *
     *   (void)uiox_tb4_probe();       (void)uiox_usb_host_probe();
     * ───────────────────────────────────────────────────────────── */

    return 0;
}

/* ═════════════════════════════════════════════════════════════════════
 * Introspection — what did the table end up holding?
 *
 * Bach's bdevsw is an array, so listing it is trivial.  A registry keyed
 * by value needs a walk, and this is that walk: it answers "is anything
 * bound?" without any caller having to know the key.
 *
 * Returns the number of bound entries; fills the caller's arrays up to
 * `max`.  Any of the out-pointers may be NULL.
 * ═════════════════════════════════════════════════════════════════════ */
int uiox_dss_list(uiox_devclass_t *cls_out, uint16_t *unit_out,
                  const char **name_out, uint16_t max)
{
    uint16_t n = 0u;

    /* UIOX_DEV_MAX is 32 (uiox_devclass.h SECTION 9) — the table is
     * bound once at probe time and never grows. */
    for (uint16_t i = 0u; i < UIOX_DEV_MAX && n < max; i++) {
        uiox_devclass_t c;
        uint16_t        u;

        if (uiox_dev_at(i, &c, &u) != 0) continue;

        if (cls_out)  cls_out[n]  = c;
        if (unit_out) unit_out[n] = u;
        if (name_out) {
            uiox_dev_dev_t *d = uiox_dev_lookup(c, u);
            name_out[n] = d ? d->name : (const char *)0;
        }
        n++;
    }
    return (int)n;
}
