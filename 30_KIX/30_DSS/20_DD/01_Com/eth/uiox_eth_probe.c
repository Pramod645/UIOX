/**
 * @file    uiox_eth_probe.c
 * @brief   Bind ethernet to the device registry.
 *
 * PATH    30_KIX/30_DSS/20_DD/01_Com/eth/uiox_eth_probe.c
 *
 * uiox_dss_init.c:97 calls uiox_eth_probe() as the family-2 STREAM entry
 * point.  THE SHAPE IS EMMC'S, transposed:
 *
 *     uiox_emmc_probe            ↔  uiox_eth_probe
 *     uiox_emmc_fwbind           ↔  uiox_netif_register  (both bind)
 *     dev->drv_priv = &_if        ↔  dev->drv_priv = &_netif
 *     uiox_emmc_if_start          ↔  uiox_netif_up
 *
 * WHY 20_DD AND NOT 34_CSS
 *
 * uiox_netif.h states its own position: "Sits between the hardware HAL
 * and the protocol stack … The protocol stack never touches hardware
 * directly; it calls uiox_netif_* functions here."  It is called BY a
 * stack, so it is a driver — 20_DD.  The stack that would call it is
 * 34_CSS's business, and may not exist yet; the bind does not wait for it.
 *
 * WHAT IS DIFFERENT FROM EMMC
 *
 * The buffer.  Family 1's blocks come from 00_bcache, keyed (dev, blkno),
 * because the filesystem decides the geometry.  Family 2's packets come
 * from 00_drvbuff's uiox_netbuf pool, unkeyed, because the DRIVER decides
 * the frame's shape — 1600 bytes of payload behind 128 bytes of headroom
 * (uiox_netbuf.h:35-37).  That is the Darwin/Windows rule: a driver
 * allocates when it defines the geometry.
 *
 * @version 1.0.0  @date 2026-10-17
 */

 #include "uiox_netif.h"      /* uiox_netif_t, uiox_netif_register, _up   */
 #include "uiox_net_hw.h"     /* uiox_net_dev_t, uiox_net_hw_init/up      */
 #include "uiox_netbuf.h"     /* uiox_netbuf_pool_init                    */
 #include "uiox_fw_i2c.h"     /* uiox_i2c_dev_t — the struct's definition*/
 #include "uiox_devclass.h"   /* UIOX_DEVCLASS_ETH, UIOX_BUS_MMIO,
                                 uiox_dev_dev_t, uiox_dev_bind            */
 #include "uiox_klibc.h"
 
 /* The HAL vtable and the driver's own ops both live in 10_DI/20_DD and
  * are bound by their own init calls; the probe does not name them. */
 
 /* ── statics — uiox_dss_init.c:27: bind() keeps the POINTER ────────── */
 static uiox_net_dev_t  s_eth_hw;
 static uiox_netif_t    s_eth_if;
 static uiox_dev_dev_t  s_eth_dev;
 static uiox_i2c_dev_t  s_eth_i2c;    /* MMIO class: unused, must exist */

 int uiox_eth_probe(void)
 {
     int rc;
     /* ── 1. HAL → FwHal ───────────────────────────────────────────
     * Mirrors uiox_emmc_fwbind().  The adapter checks
     * `if (!hw || !i2c) return -EINVAL;` and dereferences i2c, so the
     * handle must be non-NULL even though this class is MMIO and the
     * MAC's window comes from hw->base_addr.  Internally the adapter
     * calls uiox_net_hw_init(hw, &s_net_fwops), so hw->ops is set here
     * and the vtable's eleven slots are bound. */
    rc = uiox_net_fwbind(&s_eth_hw, &s_eth_i2c);
    if (rc < 0)
        return rc;
 
     /* ── 1. the buffer pool, once ─────────────────────────────────
      * 256 buffers × 1600 bytes.  uiox_netbuf_pool_init() is documented
      * "call once at boot before any netbuf use" — the probe is that
      * call, because family 2 exists only through this pool. */
     uiox_netbuf_pool_init();
 
     /* ── 2. the netif subsystem ───────────────────────────────────
      * "Initialise the netif subsystem (call once at boot)". */
     uiox_netif_subsystem_init();
 
     /* ── 3. the interface ─────────────────────────────────────────
      * Fill what the descriptor needs before it is registered.  hw is
      * the HAL device — uiox_net_hw_init() wires that half, and if the
      * class has its own fwbind (the emmc pattern) it is called here,
      * exactly as uiox_emmc_fwbind() is. */
     rc = uiox_net_hw_init(&s_eth_hw, /* ops — see the note below */ NULL);
     if (rc < 0)
         return rc;
 
     s_eth_if.hw   = &s_eth_hw;
     s_eth_if.mtu  = 1500u;              /* Ethernet II payload      */
     s_eth_if.flags = UIOX_IFF_BROADCAST | UIOX_IFF_MULTICAST;
 
     /* name is a char[16]; "eth0" is what uiox_dev_parse() maps back to
      * (UIOX_DEVCLASS_ETH, 0) — the same way "emmc0" does for family 1. */
     {
         static const char nm[] = "eth0";
         for (uint32_t i = 0; i < sizeof(nm); i++)
             s_eth_if.name[i] = nm[i];
     }
 
     rc = uiox_netif_register(&s_eth_if);
     if (rc < 0)
         return rc;
 
     /* ── 4. bring the link up ─────────────────────────────────────
      * "sets UIOX_IFF_UP, calls uiox_net_hw_up" — which itself runs
      * phy_autoneg then start.  An absent PHY fails here, and
      * uiox_dss_init.c:84 accepts that: a class that fails to probe is
      * reported and skipped. */
     rc = uiox_netif_up(&s_eth_if);
     if (rc < 0)
         return rc;
 
     /* ── 5. registry descriptor ───────────────────────────────────
      *   .cls   UIOX_DEVCLASS_ETH = UIOX_DEVCLASS_(STREAM, 0)
      *   .unit  0
      *   .bus   UIOX_BUS_MMIO — the MAC controller's window
      *   .ops   the class's own vtable.  uiox_devops.h is explicit that
      *          family 2 has NO family-level vtable: "a NIC's ring and a
      *          camera's frame queue have nothing in common but the word
      *          'buffer'."  So this points at the netif's ops, and the
      *          registry dispatch casts to uiox_net_hw_ops_t.
      *   .drv_priv  the netif — the DRIVER owns this slot, which is what
      *          makes uiox_eth_blk-style casts safe. */
     s_eth_dev.cls      = UIOX_DEVCLASS_ETH;
     s_eth_dev.unit     = 0u;
     s_eth_dev.bus      = UIOX_BUS_MMIO;
     s_eth_dev.base     = s_eth_hw.base_addr;
     s_eth_dev.irq      = s_eth_hw.irq;
     s_eth_dev.name     = "eth0";
     s_eth_dev.ops      = s_eth_hw.ops;      /* the bound HAL vtable  */
     s_eth_dev.drv_priv = &s_eth_if;
 
     /* ── 6. bind ─────────────────────────────────────────────────── */
     return uiox_dev_bind(&s_eth_dev);
 }
 