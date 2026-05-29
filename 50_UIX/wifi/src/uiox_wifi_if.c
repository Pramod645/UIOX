/**
 * @file    uiox_wifi_if.c
 * @brief   UIOX WiFi interface driver implementation.
 * @date    2026-05-28
 */

 #include "uiox_wifi_if.h"
 #include <string.h>
 #include <errno.h>
 
 int uiox_wifi_if_config(uiox_wifi_if_t *wif, uiox_wifi_hw_t *hw)
 {
     if (!wif || !hw) return -EINVAL;
     memset(wif, 0, sizeof(*wif));
     wif->hw = hw;
     wif->rate.rate_idx      = 0u;
     wif->rate.max_rate_idx  = 11u; /* e.g. MCS7 */
     wif->rate.probe_interval= 10u;
     wif->primed = true;
     return 0;
 }
 
 /* -------------------------------------------------------------------------
  * TX queue helpers
  * ---------------------------------------------------------------------- */
 
 static int txq_push(uiox_wifi_txq_t *q, uiox_wifi_frame_t *f)
 {
     if (q->count >= UIOX_WIFI_TXQUEUE_DEPTH) { q->dropped++; return -ENOSPC; }
     q->frames[q->tail % UIOX_WIFI_TXQUEUE_DEPTH] = f;
     q->tail++;
     q->count++;
     return 0;
 }
 
 static uiox_wifi_frame_t *txq_pop(uiox_wifi_txq_t *q)
 {
     if (!q->count) return NULL;
     uiox_wifi_frame_t *f = q->frames[q->head % UIOX_WIFI_TXQUEUE_DEPTH];
     q->head++;
     q->count--;
     return f;
 }
 
 /* -------------------------------------------------------------------------
  * Public API
  * ---------------------------------------------------------------------- */
 
 int uiox_wifi_if_tx(uiox_wifi_if_t *wif,
                      uiox_wifi_frame_t *frame, uint8_t ac)
 {
     if (!wif || !frame || ac >= UIOX_WIFI_AC_COUNT) return -EINVAL;
     frame->ac       = ac;
     frame->rate_idx = wif->rate.rate_idx;
     int rc = txq_push(&wif->txq[ac], frame);
     if (rc < 0) { wif->stats.tx_dropped++; }
     return rc;
 }
 
 void uiox_wifi_if_tx_flush(uiox_wifi_if_t *wif)
 {
     if (!wif || !wif->hw) return;
     const uiox_wifi_hw_ops_t *ops =
         (const uiox_wifi_hw_ops_t *)wif->hw->priv;
     if (!ops || !ops->tx_submit) return;
 
     /* Service queues in priority order: VO → VI → BE → BK */
     for (int ac = UIOX_WIFI_AC_VO; ac >= UIOX_WIFI_AC_BK; ac--) {
         uiox_wifi_frame_t *f;
         while ((f = txq_pop(&wif->txq[ac])) != NULL) {
             int rc = ops->tx_submit(wif->hw, f->paddr, f->len,
                                     f->rate_idx, f->ampdu);
             if (rc == 0) {
                 wif->stats.tx_frames++;
                 wif->stats.tx_bytes += f->len;
             } else {
                 wif->stats.tx_errors++;
                 uiox_wifi_buf_free(f);
             }
         }
     }
 }
 
 uiox_wifi_frame_t *uiox_wifi_if_rx(uiox_wifi_if_t *wif)
 {
     if (!wif || !wif->hw) return NULL;
     const uiox_wifi_hw_ops_t *ops =
         (const uiox_wifi_hw_ops_t *)wif->hw->priv;
     if (!ops || !ops->rx_poll) return NULL;
 
     uintptr_t phys   = 0;
     uint32_t  len    = 0;
     int8_t    rssi   = 0;
 
     int rc = ops->rx_poll(wif->hw, &phys, &len, &rssi);
     if (rc <= 0) return NULL;
 
     uiox_wifi_frame_t *f = uiox_wifi_buf_alloc_rx();
     if (!f) { wif->stats.rx_dropped++; return NULL; }
 
     /* Copy from DMA buffer into pool frame */
     if (len > UIOX_WIFI_FRAME_MAX) len = UIOX_WIFI_FRAME_MAX;
     memcpy(f->data, (void *)phys, len);
     f->len      = (uint16_t)len;
     f->rssi_dbm = rssi;
     f->paddr    = (uintptr_t)f->data;
 
     wif->stats.rx_frames++;
     wif->stats.rx_bytes += len;
     wif->stats.last_rssi_dbm = rssi;
     return f;
 }
 
 void uiox_wifi_if_rate_update(uiox_wifi_if_t *wif, bool success)
 {
     if (!wif) return;
     uiox_wifi_rate_ctrl_t *r = &wif->rate;
     if (success) {
         r->fail_count = 0;
         r->success_count++;
         /* Probe higher rate after probe_interval successes */
         if (r->success_count >= r->probe_interval &&
             r->rate_idx < r->max_rate_idx) {
             r->rate_idx++;
             r->success_count = 0;
         }
     } else {
         r->success_count = 0;
         r->fail_count++;
         /* Fall back after 3 consecutive failures */
         if (r->fail_count >= 3u && r->rate_idx > 0u) {
             r->rate_idx--;
             r->fail_count = 0;
             wif->stats.tx_retries++;
         }
     }
     wif->stats.tx_rate_idx = r->rate_idx;
 }
 
 void uiox_wifi_if_stats_get(const uiox_wifi_if_t *wif,
                               uiox_wifi_if_stats_t *out)
 {
     if (!wif || !out) return;
     memcpy(out, &wif->stats, sizeof(*out));
 }
 
 void uiox_wifi_if_stats_reset(uiox_wifi_if_t *wif)
 {
     if (!wif) return;
     memset(&wif->stats, 0, sizeof(wif->stats));
 }
 