/**
 * @file    uiox_nvme_probe.c
 * @brief   Bring up the NVMe controller and bind it to the registry.
 *
 * PATH    30_KIX/30_DSS/20_DD/03_NonSensors/nvme/uiox_nvme_probe.c
 *
 * The one step eMMC's probe has no equivalent for: the BAR is not a SoC
 * address the driver knows, it is assigned by PCIe.  uiox_soc_main.c runs
 * init → scan → assign_bars → enable_dev at boot from its own tc;
 * uiox_soc_pcie_get() is how this file reaches that controller, and
 * uiox_soc_pcie_find_class() locates the device by its 24-bit class code.
 */
#include "uiox_nvme_if.h"
#include "uiox_nvme_hw.h"
#include "uiox_devclass.h"
#include "uiox_devops.h"        /* uiox_blk_ops_t */
#include "uiox_klibc.h"
#include "uiox_soc_pcie.h"      /* uiox_soc_pcie_get, _find_class */
#include "uiox_fw_i2c.h"      /* struct uiox_i2c_dev — the forward decl completed */

/* The BLOCK vtable from uiox_nvme_blk.c — non-static there, so extern. */
extern const uiox_blk_ops_t s_nvme_blk_ops;

/* uiox_dev_bind() stores the POINTER it is given, not a copy
 * (uiox_devclass.h section 9), so the descriptor must outlive the call.
 * hw and if are static for the same reason — the descriptor points at
 * both.  s_nvme_i2c is a placeholder that must EXIST, not mean anything:
 * NVMe is PCIe, and fwbind rejects NULL while dereferencing nothing. */
static uiox_nvme_hw_t   s_nvme_hw;
static uiox_nvme_if_t   s_nvme_if;
static uiox_dev_dev_t   s_nvme_dev;
static struct uiox_i2c_dev s_nvme_i2c;

/* PCI class code for a non-volatile memory controller (NVM Express):
 * base class 0x01, sub-class 0x08, programming interface 0x02. */
#define UIOX_PCI_CLASS_NVME   0x010802u

int uiox_nvme_probe(void)
{
    int rc;

    /* ── 1. locate the controller on PCIe ─────────────────────────
     * The SoC layer ran init/scan/assign_bars/enable_dev at boot; this
     * only reads what it produced.  -ENODEV rather than a fabricated
     * BAR: a driver bound with bar0 = 0 would answer read() at address
     * 0, which is worse than being absent. */
    uiox_soc_pcie_ctrl_t *ctrl = uiox_soc_pcie_get();
    if (!ctrl || !ctrl->initialized) return -ENODEV;

    uiox_soc_pcie_dev_t *pdev =
        uiox_soc_pcie_find_class(ctrl, UIOX_PCI_CLASS_NVME);
    if (!pdev) return -ENODEV;              /* no NVMe controller present */
    if (!pdev->bar[0]) return -ENODEV;      /* BAR0 not assigned */

    s_nvme_hw.bar0 = (uintptr_t)pdev->bar[0];
    s_nvme_hw.irq  = pdev->irq_line;
    s_nvme_hw.caps = 0u;
    if (pdev->bar_size[0]) { /* recorded, not required */ }

    /* ── 2. HAL → FwHal ─────────────────────────────────────────── */
    rc = uiox_nvme_fwbind(&s_nvme_hw, &s_nvme_i2c);
    if (rc < 0) return rc;

    /* ── 3. driver → hardware ───────────────────────────────────── */
    rc = uiox_nvme_if_config(&s_nvme_if, &s_nvme_hw);
    if (rc < 0) return rc;

    /* ── 4. init sequence ─────────────────────────────────────────
     * power_on, enable (CC.EN ↔ CSTS.RDY), identify.  A controller that
     * never becomes ready fails here — and uiox_dss_init.c accepts that:
     * "A class that fails to probe is reported by its own return value
     * and skipped". */
    rc = uiox_nvme_if_start(&s_nvme_if);
    if (rc < 0) return rc;

    /* ── 5. descriptor — every field sourced, none invented ───────
     *   .cls      UIOX_DEVCLASS_NVME = UIOX_DEVCLASS_(BLOCK, 2)
     *   .unit     0 — first instance
     *   .bus      UIOX_BUS_PCIE — uiox_devclass.h section 3: "a block
     *             device may be MMIO (emmc), PCIe (nvme) or USB"
     *   .base     the BAR PCIe assigned, not a SoC constant
     *   .name     "nvme0" — uiox_devparse() parses that prefix (:50)
     *   .ops      the BLOCK vtable; written once, never reassigned
     *   .drv_priv the driver's own state, which is what makes
     *             uiox_nvme_blk.c's cast safe */
    s_nvme_dev.cls      = UIOX_DEVCLASS_NVME;
    s_nvme_dev.unit     = 0u;
    s_nvme_dev.bus      = UIOX_BUS_PCIE;
    s_nvme_dev.base     = s_nvme_hw.bar0;
    s_nvme_dev.irq      = s_nvme_hw.irq;
    s_nvme_dev.name     = "nvme0";
    s_nvme_dev.ops      = &s_nvme_blk_ops;
    s_nvme_dev.drv_priv = &s_nvme_if;

    /* ── 6. bind ──────────────────────────────────────────────────
     * Stores the pointer.  0, or -EEXIST if (cls, unit) is already
     * bound, -ENOSPC past UIOX_DEV_MAX (32). */
    return uiox_dev_bind(&s_nvme_dev);
}
