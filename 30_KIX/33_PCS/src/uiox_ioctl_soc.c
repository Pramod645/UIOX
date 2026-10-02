/*
 * 30_KIX/33_PCS/src/uiox_ioctl_soc.c
 *
 * The SoC / FwHal / driver ioctl handlers — kernel data → userspace.
 *
 * ── THE fd IS NOW THE FIRST ARGUMENT, AND THAT IS THE POINT ───────────
 * The draft this replaces took (cmd, uarg) with no descriptor.  That made
 * the COMMAND's magic byte do the routing a descriptor should do: any
 * process that could open ANY device node could issue ANY command here —
 * including UIOX_IOC_SYSTEM_RESET and UIOX_IOC_DMA_TRANSFER.
 *
 * ── TWO DIFFERENT FAILURES, KEPT APART ────────────────────────────────
 *   ENOTTY   the descriptor is understood and this command is not for it
 *   ENOSYS   the command is valid and this kernel has no body for it
 *   EFAULT   the user pointer did not pass the boundary test
 *   EPERM    the operation is privileged and the caller is not
 *
 * @version 1.0.0  @date 2026-10-03
 */
#include "uiox_ioctl.h"
#include "uiox_uaccess.h"
#include "uiox_klibc.h"

#define ENOSYS   38
#define EBADF     9
#define EINVAL   22
#define EFAULT   14
#define EPERM     1
#define ENOTTY   25

extern const void    *uiox_soc_get_desc(void);
extern unsigned long  uiox_soc_mem_total(void);
extern unsigned int   uiox_arch_bits(void);

extern unsigned long uiox_clk_get_hz(unsigned int clk_id);
extern int           uiox_clk_is_enabled(unsigned int clk_id);
extern void          uiox_clk_enable(unsigned int clk_id);
extern void          uiox_clk_disable(unsigned int clk_id);

extern void uiox_fw_power_reset(void);
extern void uiox_fw_power_shutdown(void);

extern int uiox_fw_dma_transfer(unsigned int  channel,
                                unsigned long src_pa,
                                unsigned long dst_pa,
                                unsigned int  len);

extern int uiox_therm_read_zone(unsigned char zone, short *temp_dc_out);
extern int uiox_bms_read_data(void *out);
extern int uiox_wifi_get_status(void *out);
extern int uiox_cam_get_frame(void *out);

extern int uiox_sec_check_ioctl(int cmd, unsigned long uarg);

static int soc_fd_ok(int fd)
{
    if (fd < 0) return 0;
    /* TODO: replace with the inode's type and major once i_major exists. */
    return 1;
}

static int cmd_magic(unsigned long cmd)
{
    return (int)((cmd >> 8) & 0xFFu);
}

long uiox_ioctl_soc_dispatch(int fd, unsigned long cmd, unsigned long uarg)
{
    if (!soc_fd_ok(fd)) return (long)-EBADF;

    switch (cmd) {

    case UIOX_IOC_GET_SOC_INFO: {
        uiox_soc_info_t k;
        unsigned char *p = (unsigned char *)&k;
        unsigned long  i;

        for (i = 0u; i < sizeof(k); i++) p[i] = 0u;

        if (uiox_soc_get_desc() != (const void *)0) {
            const char *name = (const char *)uiox_soc_get_desc();
            for (i = 0u; i < 31u && name[i] != '\0'; i++) k.name[i] = name[i];
        }

        k.cpu_hz    = (unsigned int)uiox_clk_get_hz(0u);
        k.arch_bits = uiox_arch_bits();
        k.dram_mb   = (unsigned int)(uiox_soc_mem_total() >> 20);

        if (uiox_copy_to_user((void *)uarg, &k, sizeof(k)) != 0)
            return (long)-EFAULT;
        return 0;
    }

    case UIOX_IOC_GET_CLK_HZ: {
        uiox_clk_info_t in, out;
        unsigned char  *p = (unsigned char *)&out;
        unsigned long   i;

        if (uiox_copy_from_user(&in, (const void *)uarg, sizeof(in)) != 0)
            return (long)-EFAULT;

        for (i = 0u; i < sizeof(out); i++) p[i] = 0u;

        out.clk_id  = in.clk_id;
        out.hz      = (unsigned int)uiox_clk_get_hz(in.clk_id);
        out.enabled = (unsigned char)(uiox_clk_is_enabled(in.clk_id) ? 1u : 0u);

        if (uiox_copy_to_user((void *)uarg, &out, sizeof(out)) != 0)
            return (long)-EFAULT;
        return 0;
    }

    case UIOX_IOC_CLK_ENABLE: {
        unsigned int clk_id;

        if (uiox_copy_from_user(&clk_id, (const void *)uarg,
                                sizeof(clk_id)) != 0)
            return (long)-EFAULT;

        if (uiox_sec_check_ioctl((int)cmd, uarg) != 0) return (long)-EPERM;

        uiox_clk_enable(clk_id);
        return 0;
    }

    case UIOX_IOC_CLK_DISABLE: {
        unsigned int clk_id;

        if (uiox_copy_from_user(&clk_id, (const void *)uarg,
                                sizeof(clk_id)) != 0)
            return (long)-EFAULT;

        if (uiox_sec_check_ioctl((int)cmd, uarg) != 0) return (long)-EPERM;

        uiox_clk_disable(clk_id);
        return 0;
    }

    case UIOX_IOC_GET_TEMP: {
        uiox_therm_data_t k;
        short             temp_dc = 0;
        int               rc;

        if (uiox_copy_from_user(&k, (const void *)uarg, sizeof(k)) != 0)
            return (long)-EFAULT;

        rc = uiox_therm_read_zone(k.zone, &temp_dc);
        if (rc != 0) return (long)rc;

        k.temp_dc = temp_dc;

        if (uiox_copy_to_user((void *)uarg, &k, sizeof(k)) != 0)
            return (long)-EFAULT;
        return 0;
    }

    case UIOX_IOC_GET_BMS_DATA: {
        uiox_bms_data_t k;
        int             rc;

        rc = uiox_bms_read_data(&k);
        if (rc != 0) return (long)rc;

        if (uiox_copy_to_user((void *)uarg, &k, sizeof(k)) != 0)
            return (long)-EFAULT;
        return 0;
    }

    case UIOX_IOC_GET_WIFI_STATUS: {
        uiox_wifi_status_t k;
        int                rc;

        rc = uiox_wifi_get_status(&k);
        if (rc != 0) return (long)rc;

        if (uiox_copy_to_user((void *)uarg, &k, sizeof(k)) != 0)
            return (long)-EFAULT;
        return 0;
    }

    case UIOX_IOC_CAM_GET_FRAME: {
        uiox_cam_frame_info_t k;
        int                   rc;

        rc = uiox_cam_get_frame(&k);
        if (rc != 0) return (long)rc;

        if (uiox_copy_to_user((void *)uarg, &k, sizeof(k)) != 0)
            return (long)-EFAULT;
        return 0;
    }

    case UIOX_IOC_DMA_TRANSFER: {
        uiox_dma_xfer_t k;
        int             rc;

        if (uiox_copy_from_user(&k, (const void *)uarg, sizeof(k)) != 0)
            return (long)-EFAULT;

        if (uiox_sec_check_ioctl((int)cmd, uarg) != 0) {
            k.status = 3u;
            (void)uiox_copy_to_user((void *)uarg, &k, sizeof(k));
            return (long)-EPERM;
        }

        rc = uiox_fw_dma_transfer(k.channel, k.src_pa, k.dst_pa, k.len);
        k.status = (rc == 0) ? 2u : 3u;

        if (uiox_copy_to_user((void *)uarg, &k, sizeof(k)) != 0)
            return (long)-EFAULT;
        return (long)rc;
    }

    case UIOX_IOC_SYSTEM_RESET:
        if (uiox_sec_check_ioctl((int)cmd, uarg) != 0) return (long)-EPERM;
        uiox_fw_power_reset();
        return 0;

    case UIOX_IOC_SYSTEM_OFF:
        if (uiox_sec_check_ioctl((int)cmd, uarg) != 0) return (long)-EPERM;
        uiox_fw_power_shutdown();
        return 0;

    default: {
        int m = cmd_magic(cmd);

        if (m == UIOX_IOC_MAGIC_SOC || m == UIOX_IOC_MAGIC_CLK ||
            m == UIOX_IOC_MAGIC_PWR || m == UIOX_IOC_MAGIC_THERM ||
            m == UIOX_IOC_MAGIC_BMS || m == UIOX_IOC_MAGIC_WIFI ||
            m == UIOX_IOC_MAGIC_CAM || m == UIOX_IOC_MAGIC_GPU ||
            m == UIOX_IOC_MAGIC_DMA || m == UIOX_IOC_MAGIC_KP)
            return (long)-ENOSYS;

        return (long)-ENOTTY;
    }
    }
}
