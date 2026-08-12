/*
 * 30_KIX/33_PCS/src/uiox_ioctl_soc.c
 *
 * ioctl handlers that read SoC / FwHal data and copy
 * it to userspace via uiox_copy_to_user().
 *
 * This file is the concrete implementation of the
 * kernel→userspace data crossing for SoC subsystems.
 *
 * @version 1.0.0
 * @date    2026-07-28
 */

 #include "uiox_ioctl.h"
 #include "uiox_uaccess.h"
 #include "uiox_soc.h"          /* uiox_soc_get_desc(), uiox_soc_get_clk() */
 #include "uiox_soc_clk.h"      /* uiox_clk_get_hz(), uiox_clk_enable()    */
 #include "uiox_soc_power.h"    /* uiox_fw_power_reset/shutdown/cpu_on()   */
 #include "uiox_soc_dma.h"      /* uiox_fw_dma_transfer()                  */
 #include "uiox_soc_stdio.h"    /* memset, memcpy                          */
 
 /* forward from 30_DeviceDrivers */
 extern int uiox_therm_read_zone(uint8_t zone, int16_t *temp_dc_out);
 extern int uiox_bms_read_data(uiox_bms_data_t *out);
 extern int uiox_wifi_get_status(uiox_wifi_status_t *out);
 extern int uiox_cam_get_frame(uiox_cam_frame_info_t *out);
 
 #define ENOSYS   38
 #define EINVAL   22
 #define EFAULT   14
 #define EPERM     1
 
 /* =========================================================================
  * uiox_ioctl_soc_dispatch
  *
  * Called from sys_ioctl() when the fd refers to the SoC control device.
  * Reads data from kernel/SoC/FwHal and copies to user via copy_to_user.
  * ====================================================================== */
 long uiox_ioctl_soc_dispatch(unsigned long cmd, unsigned long uarg)
 {
     switch (cmd) {
 
     /* ── UIOX_IOC_GET_SOC_INFO ────────────────────────────────────── */
     case UIOX_IOC_GET_SOC_INFO: {
         uiox_soc_info_t kinfo;
         memset(&kinfo, 0, sizeof(kinfo));
 
         /*
          * Read from FwHal: uiox_soc_get_desc() returns the
          * uiox_soc_desc_t populated by uiox_soc_arm64_detect()
          * (or the matching arch variant) at boot.
          */
         const uiox_soc_desc_t *desc = uiox_soc_get_desc();
         if (desc) {
             memcpy(kinfo.name, desc->name,
                    sizeof(kinfo.name) - 1u);
         }
         kinfo.cpu_hz    = uiox_clk_get_hz(UIOX_CLK_CPU);
         kinfo.arch_bits = (uint32_t)UIOX_ARCH_BITS;   /* from arch_defs.h */
         kinfo.dram_mb   = (uint32_t)(uiox_soc_mem_total() >> 20);
 
         /* Copy kernel struct to user pointer — the crossing point */
         if (uiox_copy_to_user((void *)uarg,
                                &kinfo,
                                sizeof(kinfo)) != 0)
             return -EFAULT;
         return 0;
     }
 
     /* ── UIOX_IOC_GET_CLK_HZ ──────────────────────────────────────── */
     case UIOX_IOC_GET_CLK_HZ: {
         uiox_clk_info_t kin, kout;
         memset(&kin,  0, sizeof(kin));
         memset(&kout, 0, sizeof(kout));
 
         /* First read user's request (which clock id they want) */
         if (uiox_copy_from_user(&kin,
                                  (const void *)uarg,
                                  sizeof(kin)) != 0)
             return -EFAULT;
 
         kout.clk_id  = kin.clk_id;
         kout.hz      = uiox_clk_get_hz((uiox_clk_id_t)kin.clk_id);
         kout.enabled = uiox_clk_is_enabled((uiox_clk_id_t)kin.clk_id);
 
         /* Copy result back to user */
         if (uiox_copy_to_user((void *)uarg,
                                &kout,
                                sizeof(kout)) != 0)
             return -EFAULT;
         return 0;
     }
 
     /* ── UIOX_IOC_CLK_ENABLE ──────────────────────────────────────── */
     case UIOX_IOC_CLK_ENABLE: {
         uint32_t clk_id = 0;
         if (uiox_copy_from_user(&clk_id,
                                  (const void *)uarg,
                                  sizeof(clk_id)) != 0)
             return -EFAULT;
         uiox_clk_enable((uiox_clk_id_t)clk_id);
         return 0;
     }
 
     /* ── UIOX_IOC_CLK_DISABLE ─────────────────────────────────────── */
     case UIOX_IOC_CLK_DISABLE: {
         uint32_t clk_id = 0;
         if (uiox_copy_from_user(&clk_id,
                                  (const void *)uarg,
                                  sizeof(clk_id)) != 0)
             return -EFAULT;
         uiox_clk_disable((uiox_clk_id_t)clk_id);
         return 0;
     }
 
     /* ── UIOX_IOC_GET_TEMP ────────────────────────────────────────── */
     case UIOX_IOC_GET_TEMP: {
         uiox_therm_data_t kd;
         memset(&kd, 0, sizeof(kd));
 
         /* Read zone number from user */
         if (uiox_copy_from_user(&kd,
                                  (const void *)uarg,
                                  sizeof(kd)) != 0)
             return -EFAULT;
 
         /*
          * Read temperature from 30_DeviceDrivers/02_Sensors/thermal
          * → uiox_therm_read_zone() reads SoC ADC via FwHal
          * → returns Q16 fixed-point converted to °C × 10
          */
         int rc = uiox_therm_read_zone(kd.zone, &kd.temp_dc);
         if (rc != 0) return (long)rc;
 
         /* Copy result back to user */
         if (uiox_copy_to_user((void *)uarg,
                                &kd,
                                sizeof(kd)) != 0)
             return -EFAULT;
         return 0;
     }
 
     /* ── UIOX_IOC_GET_BMS_DATA ────────────────────────────────────── */
     case UIOX_IOC_GET_BMS_DATA: {
         uiox_bms_data_t kd;
         memset(&kd, 0, sizeof(kd));
 
         /*
          * Read from 30_DeviceDrivers/03_NonSensors/bms
          * → reads from FwHal uioxfwbms via I2C via SoC I2C controller
          */
         int rc = uiox_bms_read_data(&kd);
         if (rc != 0) return (long)rc;
 
         if (uiox_copy_to_user((void *)uarg,
                                &kd,
                                sizeof(kd)) != 0)
             return -EFAULT;
         return 0;
     }
 
     /* ── UIOX_IOC_GET_WIFI_STATUS ─────────────────────────────────── */
     case UIOX_IOC_GET_WIFI_STATUS: {
         uiox_wifi_status_t kd;
         memset(&kd, 0, sizeof(kd));
 
         /*
          * Read from 30_DeviceDrivers/02_Sensors/WiFI
          * → uiox_wifi_get_status() reads from uiox_wifi_if_t
          * → stats populated by uiox_wifi_if_stats_get()
          */
         int rc = uiox_wifi_get_status(&kd);
         if (rc != 0) return (long)rc;
 
         if (uiox_copy_to_user((void *)uarg,
                                &kd,
                                sizeof(kd)) != 0)
             return -EFAULT;
         return 0;
     }
 
     /* ── UIOX_IOC_CAM_GET_FRAME ───────────────────────────────────── */
     case UIOX_IOC_CAM_GET_FRAME: {
         uiox_cam_frame_info_t kd;
         memset(&kd, 0, sizeof(kd));
 
         /*
          * Read from 30_DeviceDrivers/02_Sensors/camera
          * Returns paddr of DMA frame buffer.
          * User must then call mmap(fd, paddr) for zero-copy access.
          */
         int rc = uiox_cam_get_frame(&kd);
         if (rc != 0) return (long)rc;
 
         if (uiox_copy_to_user((void *)uarg,
                                &kd,
                                sizeof(kd)) != 0)
             return -EFAULT;
         return 0;
     }
 
     /* ── UIOX_IOC_DMA_TRANSFER ────────────────────────────────────── */
     case UIOX_IOC_DMA_TRANSFER: {
         uiox_dma_xfer_t kd;
         memset(&kd, 0, sizeof(kd));
 
         if (uiox_copy_from_user(&kd,
                                  (const void *)uarg,
                                  sizeof(kd)) != 0)
             return -EFAULT;
 
         /*
          * Call FwHal DMA: uiox_fw_dma_transfer()
          * src_pa and dst_pa must be validated as
          * physical addresses within DRAM bounds first.
          */
         uiox_dma_xfer_t xfer = {
             .src_pa  = kd.src_pa,
             .dst_pa  = kd.dst_pa,
             .len     = kd.len,
             .channel = kd.channel,
         };
         int rc = uiox_fw_dma_transfer(xfer.channel,
                                        xfer.src_pa,
                                        xfer.dst_pa,
                                        xfer.len);
         kd.status = (rc == 0) ? 2u : 3u;  /* done or error */
 
         if (uiox_copy_to_user((void *)uarg,
                                &kd,
                                sizeof(kd)) != 0)
             return -EFAULT;
         return (long)rc;
     }
 
     /* ── UIOX_IOC_SYSTEM_RESET ────────────────────────────────────── */
     case UIOX_IOC_SYSTEM_RESET:
         /* privileged — check process MAC label via 33_PCS/05_sec */
         uiox_fw_power_reset();
         return 0;   /* never reached */
 
     /* ── UIOX_IOC_SYSTEM_OFF ──────────────────────────────────────── */
     case UIOX_IOC_SYSTEM_OFF:
         uiox_fw_power_shutdown();
         return 0;   /* never reached */
 
     default:
         return -EINVAL;
     }
 }
 