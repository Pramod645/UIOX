/**
 * @file  uiox_als_if.h
 * @brief UIOX ALS interface driver — I²C access, IRQ, integration timer.
 * @date  2026-06-11
 */

 #ifndef UIOX_ALS_IF_H
 #define UIOX_ALS_IF_H
 
 #include "uiox_als_hw.h"
 #include "uiox_devclass.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 typedef struct {
     uint64_t  samples_read;
     uint64_t  irq_data_ready;
     uint64_t  irq_thresh_high;
     uint64_t  irq_thresh_low;
     uint32_t  gain_changes;
     uint32_t  saturations;
     uint32_t  errors;
     uint32_t  irq_count;
 } uiox_als_if_stats_t;
 
 typedef struct {
     uiox_als_hw_t      *hw;
     uiox_als_if_stats_t stats;
     bool                primed;
     bool                continuous;   /**< true = continuous, false = OS  */
     uint32_t            next_sample_ms; /**< Earliest time for next read  */
 } uiox_als_if_t;
 
 int  uiox_als_if_config      (uiox_als_if_t *aif, uiox_als_hw_t *hw,
                                bool continuous);
 int  uiox_als_if_start       (uiox_als_if_t *aif);
 void uiox_als_if_stop        (uiox_als_if_t *aif);
 
 /* Trigger a one-shot measurement (non-continuous mode) */
 int  uiox_als_if_trigger     (uiox_als_if_t *aif);
 
 /* Fetch raw counts — returns 0 if data not ready yet */
 int  uiox_als_if_fetch       (uiox_als_if_t *aif, uint32_t now_ms,
                                uint16_t *als, uint16_t *white,
                                uint16_t *ir);
 
 /* Gain / itime */
 int  uiox_als_if_set_gain    (uiox_als_if_t *aif, uiox_als_hw_gain_t g);
 int  uiox_als_if_set_itime   (uiox_als_if_t *aif, uiox_als_hw_itime_t t);
 
 /* Threshold */
 int  uiox_als_if_set_threshold(uiox_als_if_t *aif,
                                 uint16_t low, uint16_t high);
 
 /* IRQ handler — call from platform ISR or tick */
 uiox_als_evt_t *uiox_als_if_irq_handle(uiox_als_if_t *aif,
                                         uint32_t now_ms);
 
 void uiox_als_if_stats_get   (const uiox_als_if_t *aif,
                                uiox_als_if_stats_t *out);
 /* ═══════════════════════════ INSERTED — registry layer ══════════════
 *  §6 head first, then §8's bus pair.  A REG class is reached only by
 *  ioctl — 10_scfs's read branch returns SCFS_ENOTTY for a non-EVENT
 *  family, because "a lux value has no stream".
 *
 *  The number space belongs to the driver (§6); nothing above validates
 *  it.  -ENOSYS for an unknown command: klibc defines EINVAL/ENOSYS but
 *  not ENOTTY, and ENOTTY belongs to the filesystem, not the driver. */

#define UIOX_ALS_IOC_GET_RAW     0x01u   /* out: uiox_als_raw_t       */
#define UIOX_ALS_IOC_GET_WHITE   0x02u   /* out: uint16_t             */
#define UIOX_ALS_IOC_GET_IR      0x03u   /* out: uint16_t             */
#define UIOX_ALS_IOC_SET_GAIN    0x04u   /* in:  uiox_als_hw_gain_t   */
#define UIOX_ALS_IOC_SET_ITIME   0x05u   /* in:  uiox_als_hw_itime_t  */
#define UIOX_ALS_IOC_SET_THRESH  0x06u   /* in:  uiox_als_thresh_t    */
#define UIOX_ALS_IOC_GET_STATS   0x07u   /* out: uiox_als_if_stats_t  */

typedef struct { uint16_t als, white, ir; } uiox_als_raw_t;
typedef struct { uint16_t low, high; }      uiox_als_thresh_t;

typedef struct {
    uiox_dev_ops_t      head;   /* name, probe, init, deinit, ioctl  */
    uiox_busdev_ops_t   bus;    /* init, deinit, read_reg, write_reg */
} uiox_als_ops_t;

extern const uiox_als_ops_t uiox_als_class_ops;

int uiox_als_probe(void);
/* ═══════════════════════════ END INSERT ═════════════════════════ */

 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_ALS_IF_H */
 