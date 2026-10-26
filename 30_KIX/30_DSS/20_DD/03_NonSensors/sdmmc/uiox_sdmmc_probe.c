/**
 * @file    uiox_sdmmc_probe.c
 * @brief   Bring up the SD host and bind it to the registry.
 *
 * PATH    30_KIX/30_DSS/20_DD/03_NonSensors/sdmmc/uiox_sdmmc_probe.c
 *
 * The one step eMMC's probe has no equivalent for: a card may not be
 * seated.  eMMC is soldered, so its probe assumes presence; SD returns
 * -ENODEV and lets uiox_dss_init.c skip the class ("A class that fails
 * to probe is reported by its own return value and skipped").
 */
#include "uiox_sdmmc_if.h"
#include "uiox_sdmmc_hw.h"
#include "uiox_devclass.h"
#include "uiox_devops.h"      /* uiox_blk_ops_t */
#include "uiox_fw_i2c.h"      /* struct uiox_i2c_dev — the forward decl completed */
#include "uiox_klibc.h"

/* The BLOCK vtable from uiox_sdmmc_blk.c — non-static there, so extern. */
extern const uiox_blk_ops_t s_sdmmc_blk_ops;

/* uiox_dev_bind() stores the POINTER it is given, not a copy, so the
 * descriptor must outlive the call.  s_sdmmc_i2c is a placeholder that
 * must EXIST, not mean anything: SD is MMIO, and fwbind rejects NULL. */
static uiox_sdmmc_hw_t  s_sdmmc_hw;
static uiox_sdmmc_if_t  s_sdmmc_if;
static uiox_dev_dev_t   s_sdmmc_dev;
static struct uiox_i2c_dev s_sdmmc_i2c;

/* SDIO host controller base for this SoC — the same constant
 * 02_FwHal/include/uiox_fw_sd.h declares as SDIO_HC_BASE_ARM64. */
#define UIOX_SDMMC_HC_BASE_ARM64   0xFE340000u

int uiox_sdmmc_probe(void)
{
    int rc;

    /* ── 1. the host address ──────────────────────────────────────
     * A fixed SoC window, like eMMC's — no PCIe walk, so the address is
     * known before the bind rather than assigned by an enumerator. */
    s_sdmmc_hw.base = (uintptr_t)UIOX_SDMMC_HC_BASE_ARM64;
    s_sdmmc_hw.irq  = 0u;                 /* SoC IRQ routing not wired */
    s_sdmmc_hw.caps = UIOX_SDMMC_CAP_4BIT | UIOX_SDMMC_CAP_CARD_DET
                    | UIOX_SDMMC_CAP_WP_SWITCH;

    /* ── 2. HAL → FwHal ─────────────────────────────────────────── */
    rc = uiox_sdmmc_fwbind(&s_sdmmc_hw, &s_sdmmc_i2c);
    if (rc < 0) return rc;

    /* ── 3. driver → hardware ────────────────────────────────────
     * Sets .primed WITHOUT hardware I/O, so a read arriving between
     * here and step 4 would pass uiox_sdmmc_blk.c's guard and reach an
     * unclocked host.  The calls are adjacent for that reason. */
    rc = uiox_sdmmc_if_config(&s_sdmmc_if, &s_sdmmc_hw);
    if (rc < 0) return rc;

    /* ── 4. init sequence ────────────────────────────────────────
     * power_on, ident clock, IRQ enables, detect, card_init.  An empty
     * slot fails at detect with -ENODEV — accepted by dss_init, and the
     * correct answer for removable media: the class is present, the
     * medium is not. */
    rc = uiox_sdmmc_if_start(&s_sdmmc_if);
    if (rc < 0) return rc;

    /* ── 5. descriptor — every field sourced, none invented ───────
     *   .cls      UIOX_DEVCLASS_SDMMC = UIOX_DEVCLASS_(BLOCK, 1)
     *   .unit     0 — first instance
     *   .bus      UIOX_BUS_MMIO — an SDHCI register window the kernel
     *             maps, the same family of address as eMMC's
     *   .base     the SoC's SDIO host base
     *   .name     "sdmmc0" — uiox_devparse() parses that prefix (:49)
     *   .ops      the BLOCK vtable; written once, never reassigned
     *   .drv_priv the driver's state, which makes blk.c's cast safe */
    s_sdmmc_dev.cls      = UIOX_DEVCLASS_SDMMC;
    s_sdmmc_dev.unit     = 0u;
    s_sdmmc_dev.bus      = UIOX_BUS_MMIO;
    s_sdmmc_dev.base     = s_sdmmc_hw.base;
    s_sdmmc_dev.irq      = s_sdmmc_hw.irq;
    s_sdmmc_dev.name     = "sdmmc0";
    s_sdmmc_dev.ops      = &s_sdmmc_blk_ops;
    s_sdmmc_dev.drv_priv = &s_sdmmc_if;

    /* ── 6. bind ──────────────────────────────────────────────────
     * Stores the pointer.  0, or -EEXIST if (cls, unit) is already
     * bound, -ENOSPC past UIOX_DEV_MAX (32). */
    return uiox_dev_bind(&s_sdmmc_dev);
}
