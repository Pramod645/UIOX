#ifndef UIOX_FW_ETH_H
#define UIOX_FW_ETH_H
#include "uiox_fw_types.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef enum { UIOX_ETH_VIRTIO=0, UIOX_ETH_SMSC=1, UIOX_ETH_RTL8139=2 } uiox_eth_chip_t;

#define UIOX_ETH_MAC_LEN  6u
#define UIOX_ETH_MTU      1514u

typedef void (*uiox_eth_rx_cb_t)(const uint8_t *frame, uint32_t len, void *p);

typedef struct {
    uintptr_t       base;
    uint32_t        irq;
    uiox_eth_chip_t chip;
    uint8_t         mac[UIOX_ETH_MAC_LEN];
    bool            link_up;
    uint32_t        speed_mbps;
    uiox_eth_rx_cb_t rx_cb;
    void           *rx_priv;
    uint64_t        tx_bytes, rx_bytes;
    uint32_t        errors;
    bool            initialized;
    void           *priv;
} uiox_eth_dev_t;

typedef struct {
    uiox_fw_err_t (*init)   (uiox_eth_dev_t *dev);
    void          (*deinit) (uiox_eth_dev_t *dev);
    uiox_fw_err_t (*send)   (uiox_eth_dev_t *dev, const uint8_t *f, uint32_t l);
    void          (*isr)    (uiox_eth_dev_t *dev);
} uiox_eth_ops_t;

uiox_fw_err_t uiox_fw_eth_init     (uiox_eth_dev_t *dev, const uiox_eth_ops_t *ops);
void          uiox_fw_eth_deinit   (uiox_eth_dev_t *dev);
uiox_fw_err_t uiox_fw_eth_send     (uiox_eth_dev_t *dev, const uint8_t *f, uint32_t l);
void          uiox_fw_eth_set_rx_cb(uiox_eth_dev_t *dev, uiox_eth_rx_cb_t cb, void *p);
uiox_fw_err_t uiox_fw_eth_init_virtio(uiox_eth_dev_t *dev, uintptr_t base, uint32_t irq);

#ifdef __cplusplus
}
#endif
#endif /* UIOX_FW_ETH_H */
