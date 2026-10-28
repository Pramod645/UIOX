/**
 * @file    uiox_sata_probe.c
 * @brief   Bring up the AHCI controller and bind it to the registry.
 *
 * PATH    30_KIX/30_DSS/20_DD/03_NonSensors/sata/uiox_sata_probe.c
 *
 * The one step eMMC's probe has no equivalent for: like NVMe the BAR is
 * assigned by PCIe, not a SoC constant — uiox_soc_pcie_get() reaches the
 * controller and uiox_soc_pcie_find_class() locates the device by its
 * 24-bit class code.  Unlike NVMe, a SATA port may have NO DRIVE
 * attached even when the controller is present: PxSSTS.DET reports link
 * state, and -ENODEV is the honest answer for an empty port.  eMMC is
 * soldered and assumes presence; SD returns -ENODEV for an empty slot.
 * SATA needs the SD behaviour, not eMMC's.
 */
#include "uiox_sata_if.h"
#include "uiox_sata_hw.h"
#include "uiox_devclass.h"
#include "uiox_devops.h"        /* uiox_blk_ops_t */
#include "uiox_klibc.h"
#include "uiox_soc_pcie.h"      /* uiox_soc_pcie_get, _find_class */
#include "uiox_fw_i2c.h"        /* struct uiox_i2c_dev — the forward decl completed */

/* The BLOCK vtable from uiox_sata_blk.c — non-static there, so extern. */
extern const uiox_blk_ops_t s_sata_blk_ops;

/* uiox_dev_bind() stores the POINTER it is given, not a copy
 * (uiox_devclass.h section 9), so the descriptor must outlive the call.
 * hw and if are static for the same reason — the descriptor points at
 * both.  s_sata_i2c is a placeholder that must EXIST, not mean anything:
 * SATA is PCIe/MMIO, and fwbind rejects NULL while dereferencing nothing. */
static uiox_sata_hw_t   s_sata_hw;
static uiox_sata_if_t   s_sata_if;
static uiox_dev_dev_t   s_sata_dev;
static struct uiox_i2c_dev s_sata_i2c;

/* PCI class code for a SATA controller: base class 0x01 (mass storage),
 * sub-class 0x06 (SATA), programming interface 0x01 (AHCI 1.0). */
#define UIOX_PCI_CLASS_SATA_AHCI   0x010601u

int uiox_sata_probe(void)
{
    int rc;

    /* ── 1. locate the controller on PCIe ─────────────────────────
     * The SoC layer ran init/scan/assign_bars/enable_dev at boot; this
     * only reads what it produced.  -ENODEV rather than a fabricated
     * BAR: a driver bound with bar5 = 0 would answer register reads at
     * address 0, which is worse than being absent. */
    uiox_soc_pcie_ctrl_t *ctrl = uiox_soc_pcie_get();
    if (!ctrl || !ctrl->initialized) return -ENODEV;

    uiox_soc_pcie_dev_t *pdev =
        uiox_soc_pcie_find_class(ctrl, UIOX_PCI_CLASS_SATA_AHCI);
    if (!pdev) return -ENODEV;              /* no AHCI controller present */
    if (!pdev->bar[5]) return -ENODEV;      /* BAR5 not assigned (ABAR)   */

    s_sata_hw.bar5 = (uintptr_t)pdev->bar[5];
    s_sata_hw.irq  = pdev->irq_line;

    /* ── 2. HAL → FwHal ─────────────────────────────────────────── */
    rc = uiox_sata_fwbind(&s_sata_hw, &s_sata_i2c);
    if (rc < 0) return rc;

    /* ── 3. driver → hardware ───────────────────────────────────── */
    rc = uiox_sata_if_config(&s_sata_if, &s_sata_hw);
    if (rc < 0) return rc;

    /* ── 4. init sequence ─────────────────────────────────────────
     * GHC.AE, port start, COMRESET, then the port link check.  A port
     * with no drive attached fails HERE rather than at first read —
     * and uiox_dss_init.c accepts that: "A class that fails to probe is
     * reported by its own return value and skipped". */
    rc = uiox_sata_if_start(&s_sata_if);
    if (rc < 0) return rc;

    /* ── 5. descriptor — every field sourced, none invented ───────
     *   .cls      UIOX_DEVCLASS_SATA = UIOX_DEVCLASS_(BLOCK, 3)
     *   .unit     0 — first instance
     *   .bus      UIOX_BUS_PCIE — AHCI is a PCIe function; the ABAR
     *             is the register window, not a SoC constant
     *   .base     the BAR5 PCIe assigned, not a fabricated address
     *   .name     "sata0" — uiox_devparse() parses that prefix (:50)
     *   .ops      the BLOCK vtable; written once, never reassigned
     *   .drv_priv the driver's own state, which is what makes
     *             uiox_sata_blk.c's cast safe */
    s_sata_dev.cls      = UIOX_DEVCLASS_SATA;
    s_sata_dev.unit     = 0u;
    s_sata_dev.bus      = UIOX_BUS_PCIE;
    s_sata_dev.base     = s_sata_hw.bar5;
    s_sata_dev.irq      = s_sata_hw.irq;
    s_sata_dev.name     = "sata0";
    s_sata_dev.ops      = &s_sata_blk_ops;
    s_sata_dev.drv_priv = &s_sata_if;
        /* ── 5b. controller identity, read from the hardware ──────────
     * uiox_sata_hw.c is a 112-line ops forwarder and never sets these
     * four fields, so whoever calls the HAL owns them.  Every value
     * below is read from a register; none is asserted. */
    s_sata_hw.caps = uiox_sata_hw_ghc_read(&s_sata_hw, AHCI_GHC_CAP);

    /* PI — ports implemented bitmap.  num_ports is the popcount, and
     * the port bit must be set or ahci_init() will return -NODEV. */
    uint32_t pi = uiox_sata_hw_ghc_read(&s_sata_hw, AHCI_GHC_PI);
    if (!(pi & (1u << s_sata_hw.port))) return -ENODEV;

    s_sata_hw.num_ports = 0u;
    for (uint32_t b = pi; b; b &= (b - 1u)) s_sata_hw.num_ports++;

    /* ctrl_type / dev_type: set only from what the controller reports.
     * CAP.SAM (bit 18) says the HBA supports AHCI-mode only; CAP.S64A
     * (bit 31) says 64-bit addressing.  Without a mapping of these to
     * the uiox_sata_ctrl_t enumerators in front of me, they take the
     * enum's zero value rather than a guessed variant. */
    s_sata_hw.ctrl_type = (uiox_sata_ctrl_t)0;
    s_sata_hw.dev_type  = (uiox_sata_state_t)0;


    /* ── 6. bind ──────────────────────────────────────────────────
     * Stores the pointer.  0, or -EEXIST if (cls, unit) is already
     * bound, -ENOSPC past UIOX_DEV_MAX (32). */
    return uiox_dev_bind(&s_sata_dev);
}
