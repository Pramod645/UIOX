/*
 * 34_CSS/01_Com/gpu/uiox_kix_css_gpu.h
 *
 * GPU's four entry points — declared here, defined in the .c beside it.
 * Same rule as uiox_kix_css_hdmi.h: the family declares its own surface.
 *
 * @version 1.0.0  @date 2026-11-21
 */
#ifndef UIOX_KIX_CSS_GPU_H
#define UIOX_KIX_CSS_GPU_H

#include "uiox_kix_sccss.h"
#include "uiox_gpu_subsys.h"

int64_t uiox_kix_css_gpu_state(void);

int64_t uiox_kix_css_gpu_stats(uiox_uint64_t out_addr,
                               uiox_uint64_t out_len);

int64_t uiox_kix_css_gpu_pso  (uiox_uint64_t op,
                               uiox_uint64_t pso_id,
                               uiox_uint64_t desc_addr);

int64_t uiox_kix_css_gpu_frame(uiox_uint64_t cmdbuf_addr,
                               uiox_uint64_t cmdbuf_len,
                               uiox_uint64_t wait_ms);

#endif /* UIOX_KIX_CSS_GPU_H */
