/* ─────────────────────────────────────────────────────────────────────────
 * uiox_fw_tb4.h — Thunderbolt 4 / USB4 (Intel JHL8540)
 * ───────────────────────────────────────────────────────────────────────── */
#ifndef UIOX_FW_TB4_H
#define UIOX_FW_TB4_H
#include "uiox_fw_types.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef enum { UIOX_TB4_VER_TB3=0, UIOX_TB4_VER_TB4=1,
               UIOX_TB4_VER_USB4V1=2, UIOX_TB4_VER_USB4V2=3 } uiox_tb4_ver_t;

#define UIOX_TB4_CAP_40GBPS     (1u << 0)
#define UIOX_TB4_CAP_PCIE       (1u << 1)
#define UIOX_TB4_CAP_DP         (1u << 2)
#define UIOX_TB4_CAP_USB3       (1u << 3)
#define UIOX_TB4_CAP_POWER_100W (1u << 4)

typedef enum { UIOX_TB4_SEC_NONE=0, UIOX_TB4_SEC_USER=1,
               UIOX_TB4_SEC_SECURE=2 } uiox_tb4_sec_t;

typedef struct {
    uintptr_t      nhi_base;   /**< NHI MMIO BAR0                     */
    uintptr_t      cfg_base;   /**< Configuration space BAR1          */
    uint32_t       irq;
    uint32_t       caps;
    uiox_tb4_ver_t version;
    uiox_tb4_sec_t security;
    char           model[32];
    uint8_t        num_ports;
    bool           connected;
    bool           initialized;
    void          *priv;
} uiox_tb4_dev_t;

typedef struct {
    uiox_fw_err_t (*init)        (uiox_tb4_dev_t *dev);
    void          (*deinit)      (uiox_tb4_dev_t *dev);
    uiox_fw_err_t (*power_on)    (uiox_tb4_dev_t *dev);
    void          (*power_off)   (uiox_tb4_dev_t *dev);
    uiox_fw_err_t (*icm_send)    (uiox_tb4_dev_t *dev,
                                    const uint32_t *msg, uint8_t n);
    uiox_fw_err_t (*icm_recv)    (uiox_tb4_dev_t *dev,
                                    uint32_t *msg, uint8_t max);
    uiox_fw_err_t (*approve_dev) (uiox_tb4_dev_t *dev,
                                    uint8_t route_hi, uint32_t route_lo);
    bool          (*hotplug_pend)(uiox_tb4_dev_t *dev);
    void          (*isr)         (uiox_tb4_dev_t *dev);
} uiox_tb4_ops_t;

uiox_fw_err_t uiox_fw_tb4_init       (uiox_tb4_dev_t *dev, const uiox_tb4_ops_t *ops);
void          uiox_fw_tb4_deinit     (uiox_tb4_dev_t *dev);
uiox_fw_err_t uiox_fw_tb4_power_on   (uiox_tb4_dev_t *dev);
void          uiox_fw_tb4_power_off  (uiox_tb4_dev_t *dev);
uiox_fw_err_t uiox_fw_tb4_approve_dev(uiox_tb4_dev_t *dev,
                                        uint8_t route_hi, uint32_t route_lo);
uiox_fw_err_t uiox_fw_tb4_init_jhl8540(uiox_tb4_dev_t *dev,
                                         uintptr_t nhi_base, uint32_t irq);
#ifdef __cplusplus
}
#endif
#endif /* UIOX_FW_TB4_H */
