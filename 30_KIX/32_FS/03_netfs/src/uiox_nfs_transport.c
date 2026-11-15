/**
 * @file  30_KIX/32_FS/03_netfs/src/uiox_nfs_transport.c
 * @brief The wire a network filesystem is mounted over.
 *
 * netfs does not know what its link is.  uiox_nfs_mount_params_t takes
 * net_send / net_recv as function pointers and net_ctx as void*
 * (uiox_nfs_vfs.h:76-79), and both protocol clients call them through
 * that context — 9P at uiox_nfs_9p.c:56,60, RPC through the same pair.
 * These two functions ARE the link.
 *
 * ── why the HARDWARE level, not netif ───────────────────────────────
 * netif offers a higher path, but netif->output() takes a dst_ip4
 * (uiox_netif.h:114) and the protocol stack that would supply one is
 * forward-declared only — uiox_netif.h:192 says "not-yet-generated".
 * netfs needs a byte stream to a peer, not an IP connection, so raw
 * frames are the shorter and more honest route.
 *
 *   TX   ops->tx_submit(dev, buf_phys, len)     uiox_net_hw.h:148
 *   RX   ops->rx_poll  (dev, buf, maxlen)       uiox_net_hw.h:159
 *
 * ── why there is no address conversion ──────────────────────────────
 * tx_submit's comment says "@param buf  Physical address of packet
 * data".  No virt_to_phys exists anywhere in 30_KIX or 10_BSP — and one
 * is not needed: this kernel is IDENTITY-MAPPED for kernel memory.
 * uiox_kix_page_fault.c:444 dereferences
 *
 *     (void *)(uintptr_t)(newpfd->pfd_pfn << UIOX_PAGE_SHIFT)
 *
 * as a pointer, which only works if the kernel virtual address equals
 * the physical one.  So a KERNEL buffer's address IS its physical
 * address, and passing it to tx_submit is correct.
 *
 * THIS DOES NOT HOLD FOR A USERLAND BUFFER.  A buffer reached through a
 * process's own mapping at EL0 is not guaranteed to be identity-mapped.
 * The first transport sends and receives kernel buffers, which is what
 * netfs does; a mount serving a userspace client would need the
 * translation this file deliberately does not perform.
 *
 * ── the timeout ─────────────────────────────────────────────────────
 * net_recv polls, because rx_poll is a PULL interface that returns 0
 * when no packet is ready (uiox_net_hw.h:159).  The bound is counted,
 * not timed, because no monotonic clock is wired yet — the same gap
 * uiox_nfs_vfs.c:100 names for the page cache ("stub uptime — real
 * kernel wires timer here").  When that clock lands, this loop should
 * use it and the parameter becomes a real millisecond deadline.
 *
 * @version 1.0.0  @date  2026-11-15
 */

 #include "uiox_nfs_types.h"
 #include "uiox_net_hw.h"        /* uiox_net_dev_t, uiox_net_hw_ops_t,
                                  * UIOX_HW_MTU_ETHERNET */
 
 /* ── uiox_nfs_net_send ────────────────────────────────────────────────
  * Hand one frame to the NIC.
  *
  * Returns UIOX_NFS_OK, or:
  *   NODEV    no device, no ops vtable, or no tx_submit entry
  *   INVAL    len is zero or above the Ethernet MTU
  *   IO       the driver refused the frame (ring full, or a bus error)
  */
 uiox_nfs_err_t uiox_nfs_net_send(void *net_ctx,
                                  const uint8_t *buf, uint32_t len)
 {
     uiox_net_dev_t          *dev = (uiox_net_dev_t *)net_ctx;
     const uiox_net_hw_ops_t *ops;
     int                      rc;
 
     if (!dev || !buf || len == 0u)  return UIOX_NFS_ERR_INVAL;
 
     /* dev->ops is declared `const void *` (uiox_net_hw.h:114), not a
      * typed vtable pointer — the cast is the declaration's shape, not a
      * conversion. */
     if (!dev->ops)                  return UIOX_NFS_ERR_NODEV;
     ops = (const uiox_net_hw_ops_t *)dev->ops;
 
     if (!ops->tx_submit)            return UIOX_NFS_ERR_NODEV;
     if (len > UIOX_HW_MTU_ETHERNET) return UIOX_NFS_ERR_INVAL;
 
     /* buf's address IS its physical address — see the header note. */
     rc = ops->tx_submit(dev, (uintptr_t)buf, (uint16_t)len);
     if (rc != 0)
         return UIOX_NFS_ERR_IO;
 
     /* Reclaim completed descriptors so the ring does not fill under a
      * long exchange.  Optional: a driver that reclaims in its ISR does
      * not need this, and the NULL test says so rather than assuming. */
     if (ops->tx_reclaim)
         ops->tx_reclaim(dev);
 
     return UIOX_NFS_OK;
 }
 
 /* ── uiox_nfs_net_recv ────────────────────────────────────────────────
  * Pull one frame, waiting up to timeout_ms.
  *
  * Returns UIOX_NFS_OK with *got set to the byte count, or:
  *   NODEV    no device, no ops vtable, or no rx_poll entry
  *   INVAL    no output pointer, or max is 0
  *   IO       the driver reported an error
  *   TIMEOUT  nothing arrived before the bound
  *
  * A successful return with *got == 0 is not produced; the caller can
  * treat OK as "bytes are in the buffer".
  */
 uiox_nfs_err_t uiox_nfs_net_recv(void *net_ctx,
                                  uint8_t *buf, uint32_t max,
                                  uint32_t *got, uint32_t timeout_ms)
 {
     uiox_net_dev_t          *dev = (uiox_net_dev_t *)net_ctx;
     const uiox_net_hw_ops_t *ops;
     uint32_t                 waited = 0u;
     int                      n;
 
     if (!dev || !buf || !got || max == 0u) return UIOX_NFS_ERR_INVAL;
 
     if (!dev->ops)                    return UIOX_NFS_ERR_NODEV;
     ops = (const uiox_net_hw_ops_t *)dev->ops;
 
     if (!ops->rx_poll)                return UIOX_NFS_ERR_NODEV;
 
     *got = 0u;
 
     /* rx_poll is a PULL: >0 bytes, 0 "nothing ready", <0 error
      * (uiox_net_hw.h:159).  The idiom is the one uiox_arp_resolve()
      * already uses — bounded wait, distinct error on expiry. */
     for (;;) {
         n = ops->rx_poll(dev, buf, (uint16_t)max);
 
         if (n > 0) {
             *got = (uint32_t)n;
             return UIOX_NFS_OK;
         }
         if (n < 0)
             return UIOX_NFS_ERR_IO;
 
         if (waited >= timeout_ms)
             return UIOX_NFS_ERR_TIMEOUT;
 
         waited++;
 
         /* No clock to sleep on yet — see the header note.  When the
          * kernel's timer lands this becomes a real wait, and a bare
          * spin here would be the one place a mount blocks a core. */
     }
 }
 
 /* ── the context these are given ──────────────────────────────────────
  * The caller fills uiox_nfs_mount_params_t.net_ctx with the device, and
  * net_send / net_recv with these two:
  *
  *     uiox_net_dev_t *dev = uiox_netif_find("eth0")->hw;   /x or the
  *                                        probe's own device handle x/
  *     mp.net_ctx  = dev;
  *     mp.net_send = uiox_nfs_net_send;
  *     mp.net_recv = uiox_nfs_net_recv;
  *
  * net_ctx is void* in the params struct, so nothing in netfs names
  * uiox_net_dev_t — the type is this file's business.
  */
 
 /* ***This is End of file, there is no more line should be added after this line*** */
 