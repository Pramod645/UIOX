/**
 * @file  uiox_fw_devsw.h
 * @brief UIOX Firmware — Device switch table (char + block).
 *        Mirrors 30_DeviceDrivers/iossytem.md: devsw.h / superblock.h.
 *        Maps UIOX device major numbers to driver ops tables.
 * @version 1.0.0
 * @date    2026-06-21
 */

 #ifndef UIOX_FW_DEVSW_H
 #define UIOX_FW_DEVSW_H
 
 #include "uiox_fw_types.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Device numbers (major) — mirrors UIOX 30_DeviceDrivers layout
  * ====================================================================== */
 
 /* Character devices */
 #define UIOX_DEV_MAJOR_CONSOLE  0u   /**< /dev/console (UART)            */
 #define UIOX_DEV_MAJOR_TTY      1u   /**< /dev/ttyN    (serial TTY)      */
 #define UIOX_DEV_MAJOR_PTY      2u   /**< /dev/ptyN    (pseudo-TTY)      */
 #define UIOX_DEV_MAJOR_NULL     3u   /**< /dev/null                      */
 #define UIOX_DEV_MAJOR_ZERO     4u   /**< /dev/zero                      */
 #define UIOX_DEV_MAJOR_MEM      5u   /**< /dev/mem                       */
 #define UIOX_DEV_MAJOR_GPIO     6u   /**< /dev/gpio                      */
 #define UIOX_DEV_MAJOR_I2C      7u   /**< /dev/i2c                       */
 #define UIOX_DEV_MAJOR_SPI      8u   /**< /dev/spi                       */
 #define UIOX_DEV_MAJOR_SENSOR   9u   /**< /dev/sensor                    */
 
 /* Block devices */
 #define UIOX_DEV_MAJOR_RAM      16u  /**< /dev/ram0                      */
 #define UIOX_DEV_MAJOR_SD       17u  /**< /dev/mmcblk0                   */
 #define UIOX_DEV_MAJOR_EMMC     18u  /**< /dev/mmcblk1                   */
 #define UIOX_DEV_MAJOR_NVME     19u  /**< /dev/nvme0n1                   */
 #define UIOX_DEV_MAJOR_IDE      20u  /**< /dev/hda                       */
 #define UIOX_DEV_MAJOR_VIRTBLK  21u  /**< /dev/vda                       */
 
 #define UIOX_FW_DEVSW_MAX_CHAR  32u
 #define UIOX_FW_DEVSW_MAX_BLOCK 32u
 
 /* =========================================================================
  * Buffer header — from 31_BufferCache (matches uiox_fs dev_types.h)
  * ====================================================================== */
 
 #define UIOX_FW_BUF_SIZE        512u
 #define UIOX_FW_BUF_LOCKED      UIOX_FW_BIT(0)
 #define UIOX_FW_BUF_VALID       UIOX_FW_BIT(1)
 #define UIOX_FW_BUF_DIRTY       UIOX_FW_BIT(2)
 #define UIOX_FW_BUF_WRITING     UIOX_FW_BIT(3)
 #define UIOX_FW_BUF_READING     UIOX_FW_BIT(4)
 
 typedef struct uiox_fw_buf {
     uint32_t            flags;
     uint32_t            dev;         /**< UIOX_DEV_MAJOR_* device number */
     uint64_t            blkno;       /**< Block number                   */
     uint8_t             data[UIOX_FW_BUF_SIZE];
     struct uiox_fw_buf *b_next;      /**< Hash chain                     */
     struct uiox_fw_buf *av_next;     /**< Free list                      */
     struct uiox_fw_buf *av_prev;
 } uiox_fw_buf_t;
 
 /* =========================================================================
  * Character device switch entry
  * ====================================================================== */
 
 typedef struct {
     const char   *name;
     uint32_t      major;
     /* open(minor) → UIOX_FW_OK or error */
     uiox_fw_err_t (*copen) (uint32_t minor, int flags);
     /* close(minor) */
     void          (*cclose)(uint32_t minor);
     /* read/write one byte */
     int           (*cread) (uint32_t minor);
     uiox_fw_err_t (*cwrite)(uint32_t minor, char c);
     /* ioctl */
     uiox_fw_err_t (*cioctl)(uint32_t minor, uint32_t cmd,
                              uintptr_t arg);
     bool          ready;
 } uiox_fw_cdevsw_t;
 
 /* =========================================================================
  * Block device switch entry
  * ====================================================================== */
 
 typedef struct {
     const char   *name;
     uint32_t      major;
     uiox_fw_err_t (*bopen)    (uint32_t minor, int flags);
     void          (*bclose)   (uint32_t minor);
     /* strategy: issue async read/write of buf */
     void          (*strategy) (uiox_fw_buf_t *buf);
     /* ioctl */
     uiox_fw_err_t (*bioctl)   (uint32_t minor, uint32_t cmd,
                                 uintptr_t arg);
     uint64_t      num_blocks;
     uint32_t      block_size;
     bool          ready;
 } uiox_fw_bdevsw_t;
 
 /* =========================================================================
  * Device switch table
  * ====================================================================== */
 
 typedef struct {
     uint32_t          magic;
     uiox_fw_cdevsw_t  cdev[UIOX_FW_DEVSW_MAX_CHAR];
     uiox_fw_bdevsw_t  bdev[UIOX_FW_DEVSW_MAX_BLOCK];
     uint32_t          cdev_count;
     uint32_t          bdev_count;
 } uiox_fw_devsw_t;
 
 /* API */
 uiox_fw_err_t  uiox_fw_devsw_init     (uiox_fw_devsw_t *dsw);
 uiox_fw_err_t  uiox_fw_cdev_register  (uiox_fw_devsw_t *dsw,
                                          const uiox_fw_cdevsw_t *entry);
 uiox_fw_err_t  uiox_fw_bdev_register  (uiox_fw_devsw_t *dsw,
                                          const uiox_fw_bdevsw_t *entry);
 uiox_fw_cdevsw_t *uiox_fw_cdev_get    (uiox_fw_devsw_t *dsw,
                                           uint32_t major);
 uiox_fw_bdevsw_t *uiox_fw_bdev_get    (uiox_fw_devsw_t *dsw,
                                           uint32_t major);
 void           uiox_fw_devsw_print    (const uiox_fw_devsw_t *dsw);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_FW_DEVSW_H */
 