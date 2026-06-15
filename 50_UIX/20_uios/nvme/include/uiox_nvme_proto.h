/**
 * @file  uiox_nvme_proto.h
 * @brief UIOX NVMe protocol — Identify, queue creation, SMART, features.
 * @date  2026-06-12
 */

 #ifndef UIOX_NVME_PROTO_H
 #define UIOX_NVME_PROTO_H
 
 #include "uiox_nvme_if.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 typedef enum {
     UIOX_NVME_INIT_IDLE         = 0,
     UIOX_NVME_INIT_RESET,
     UIOX_NVME_INIT_ENABLE,
     UIOX_NVME_INIT_IDENTIFY_CTRL,
     UIOX_NVME_INIT_CREATE_IO_Q,
     UIOX_NVME_INIT_IDENTIFY_NS,
     UIOX_NVME_INIT_SET_FEATURES,
     UIOX_NVME_INIT_DONE,
     UIOX_NVME_INIT_ERROR,
 } uiox_nvme_init_state_t;
 
 typedef struct {
     uiox_nvme_if_t        *nif;
     uiox_nvme_init_state_t init_state;
     bool                   initialized;
 } uiox_nvme_proto_t;
 
 int  uiox_nvme_proto_init         (uiox_nvme_proto_t *proto,
                                     uiox_nvme_if_t *nif);
 int  uiox_nvme_proto_ctrl_init    (uiox_nvme_proto_t *proto);
 
 /* Admin: Identify */
 int  uiox_nvme_proto_identify_ctrl(uiox_nvme_proto_t *proto);
 int  uiox_nvme_proto_identify_ns  (uiox_nvme_proto_t *proto,
                                     uint32_t nsid);
 int  uiox_nvme_proto_ns_list      (uiox_nvme_proto_t *proto);
 
 /* Admin: Queue management */
 int  uiox_nvme_proto_create_iocq  (uiox_nvme_proto_t *proto,
                                     uint16_t qid, uint16_t size,
                                     uint16_t vec);
 int  uiox_nvme_proto_create_iosq  (uiox_nvme_proto_t *proto,
                                     uint16_t qid, uint16_t size,
                                     uint16_t cqid);
 int  uiox_nvme_proto_delete_iocq  (uiox_nvme_proto_t *proto,
                                     uint16_t qid);
 int  uiox_nvme_proto_delete_iosq  (uiox_nvme_proto_t *proto,
                                     uint16_t qid);
 
 /* Admin: Features */
 int  uiox_nvme_proto_set_queues   (uiox_nvme_proto_t *proto,
                                     uint16_t nsq, uint16_t ncq);
 int  uiox_nvme_proto_set_apst     (uiox_nvme_proto_t *proto, bool enable);
 int  uiox_nvme_proto_set_volatile_wc(uiox_nvme_proto_t *proto, bool en);
 
 /* Admin: SMART / health log */
 int  uiox_nvme_proto_smart_log    (uiox_nvme_proto_t *proto,
                                     uint8_t *buf);
 
 /* I/O commands */
 int  uiox_nvme_proto_read         (uiox_nvme_proto_t *proto,
                                     uint32_t nsid, uint64_t slba,
                                     uint8_t *buf, uint32_t nlb);
 int  uiox_nvme_proto_write        (uiox_nvme_proto_t *proto,
                                     uint32_t nsid, uint64_t slba,
                                     const uint8_t *buf, uint32_t nlb);
 int  uiox_nvme_proto_flush        (uiox_nvme_proto_t *proto,
                                     uint32_t nsid);
 int  uiox_nvme_proto_trim         (uiox_nvme_proto_t *proto,
                                     uint32_t nsid, uint64_t slba,
                                     uint32_t nlb);
 
 /* Admin: Format NVM */
 int  uiox_nvme_proto_format_ns    (uiox_nvme_proto_t *proto,
                                     uint32_t nsid, uint8_t lbaf);
 
 /* Shutdown */
 int  uiox_nvme_proto_shutdown     (uiox_nvme_proto_t *proto);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_NVME_PROTO_H */
 