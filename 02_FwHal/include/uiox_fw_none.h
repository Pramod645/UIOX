/*
 * 02_FwHal/include/uiox_fw_none.h
 * Template + safe fallback storage driver.
 */
#ifndef UIOX_FW_NONE_H
#define UIOX_FW_NONE_H
#include "uiox_fw_types.h"
#include "uiox_fw_storage.h"
#ifdef __cplusplus
extern "C" {
#endif
uiox_fw_err_t uiox_fw_none_init(void);
uiox_fw_stor_dev_t *uiox_fw_none_stor_dev(void);
#ifdef __cplusplus
}
#endif
#endif /* UIOX_FW_NONE_H */
