/**
 * @file  uiox_pkg_types.h
 * @brief UIOX Package Manager — base types, package header, error codes.
 *
 * Integrates with:
 *   32_FileSystem  — block read/write via buf_read()/buf_write()
 *   31_BufferCache — buffer cache for package index sectors
 *   40_SystemCallInterface — pkg_install/pkg_remove syscall dispatch
 *
 * @version 1.0.0
 * @date    2026-06-29
 */

 #ifndef UIOX_PKG_TYPES_H
 #define UIOX_PKG_TYPES_H
 
 #include <stdint.h>
 #include <stddef.h>
 #include <stdbool.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Portable integer types (mirrors uiox_boot_types.h convention)
  * ====================================================================== */
 
 #ifndef UIOX_TYPES_DEFINED
 #define UIOX_TYPES_DEFINED
 typedef unsigned char       uint8_t;
 typedef unsigned short      uint16_t;
 typedef unsigned int        uint32_t;
 typedef unsigned long long  uint64_t;
 typedef signed   char       int8_t;
 typedef signed   short      int16_t;
 typedef signed   int        int32_t;
 typedef signed   long long  int64_t;
 //typedef uint64_t            uintptr_t;
 //typedef uint64_t            size_t;
 #endif
 
 /* =========================================================================
  * Error codes
  * ====================================================================== */
 
 typedef enum {
     UIOX_PKG_OK              =  0,
     UIOX_PKG_ERR_GENERIC     = -1,
     UIOX_PKG_ERR_INVAL       = -2,
     UIOX_PKG_ERR_NOMEM       = -3,
     UIOX_PKG_ERR_IO          = -4,
     UIOX_PKG_ERR_NOTFOUND    = -5,
     UIOX_PKG_ERR_ALREADY     = -6,  /**< Package already installed       */
     UIOX_PKG_ERR_CONFLICT    = -7,  /**< Dependency conflict             */
     UIOX_PKG_ERR_BADMAGIC    = -8,  /**< Corrupt package header          */
     UIOX_PKG_ERR_BADCSUM     = -9,  /**< SHA-256 mismatch               */
     UIOX_PKG_ERR_DEPFAIL     = -10, /**< Dependency resolution failed    */
     UIOX_PKG_ERR_OVERFLOW    = -11, /**< Buffer / registry full          */
     UIOX_PKG_ERR_PERM        = -12, /**< Permission denied               */
     UIOX_PKG_ERR_BUSY        = -13, /**< Package locked by another op    */
     UIOX_PKG_ERR_UNSUP       = -14, /**< Unsupported package format      */
 } uiox_pkg_err_t;
 
 /* =========================================================================
  * Magic numbers
  * ====================================================================== */
 
 #define UIOX_PKG_MAGIC          0x55504B47u  /**< "UPKG"                  */
 #define UIOX_PKG_INDEX_MAGIC    0x55504958u  /**< "UPIX"                  */
 #define UIOX_PKG_VERSION        0x00010000u  /**< v1.0.0                  */
 
 /* =========================================================================
  * Package version: major.minor.patch packed into uint32_t
  * ====================================================================== */
 
 #define UIOX_PKG_VER(maj,min,pat) \
     (((uint32_t)(maj) << 16) | ((uint32_t)(min) << 8) | (uint32_t)(pat))
 
 #define UIOX_PKG_VER_MAJOR(v)   (((v) >> 16) & 0xFFu)
 #define UIOX_PKG_VER_MINOR(v)   (((v) >>  8) & 0xFFu)
 #define UIOX_PKG_VER_PATCH(v)   ( (v)         & 0xFFu)
 
 /* =========================================================================
  * String field sizes
  * ====================================================================== */
 
 #define UIOX_PKG_NAME_MAX       48u   /**< Package name length            */
 #define UIOX_PKG_DESC_MAX       128u  /**< Short description              */
 #define UIOX_PKG_URL_MAX        128u  /**< Repository URL                 */
 #define UIOX_PKG_ARCH_MAX       16u   /**< Target architecture string     */
 #define UIOX_PKG_SHA256_LEN     32u   /**< SHA-256 digest bytes           */
 #define UIOX_PKG_MAX_DEPS       16u   /**< Max direct dependencies        */
 #define UIOX_PKG_MAX_FILES      256u  /**< Max files per package          */
 #define UIOX_PKG_PATH_MAX       128u  /**< Install path length            */
 
 /* =========================================================================
  * Dependency descriptor
  * ====================================================================== */
 
 typedef enum {
     UIOX_PKG_DEP_REQUIRED   = 0,  /**< Must be installed                 */
     UIOX_PKG_DEP_OPTIONAL   = 1,  /**< Installed if available            */
     UIOX_PKG_DEP_CONFLICT   = 2,  /**< Must NOT be installed             */
     UIOX_PKG_DEP_PROVIDES   = 3,  /**< This package provides the name    */
 } uiox_pkg_dep_type_t;
 
 typedef struct {
     char                 name[UIOX_PKG_NAME_MAX];
     uint32_t             ver_min;      /**< Minimum version (inclusive)   */
     uint32_t             ver_max;      /**< Maximum version (0 = any)     */
     uiox_pkg_dep_type_t  type;
 } uiox_pkg_dep_t;
 
 /* =========================================================================
  * Installed file record
  * ====================================================================== */
 
 typedef struct {
     char     path[UIOX_PKG_PATH_MAX]; /**< Absolute install path         */
     uint32_t size;                     /**< File size in bytes            */
     uint8_t  sha256[UIOX_PKG_SHA256_LEN]; /**< Per-file checksum         */
     uint8_t  permissions;             /**< rwxrwxrwx bits (low 9)        */
 } uiox_pkg_file_t;
 
 /* =========================================================================
  * Package header (on-disk and in-memory)
  * Stored as first sector(s) of each .upkg archive.
  * ====================================================================== */
 
 typedef struct __attribute__((packed)) {
     uint32_t magic;                     /**< UIOX_PKG_MAGIC               */
     uint32_t format_version;            /**< UIOX_PKG_VERSION             */
     char     name   [UIOX_PKG_NAME_MAX];
     char     desc   [UIOX_PKG_DESC_MAX];
     char     arch   [UIOX_PKG_ARCH_MAX];
     char     url    [UIOX_PKG_URL_MAX];
     uint32_t version;                   /**< Packed UIOX_PKG_VER()        */
     uint32_t installed_size;            /**< Bytes on disk after install  */
     uint32_t archive_size;              /**< Compressed archive bytes     */
     uint8_t  sha256[UIOX_PKG_SHA256_LEN]; /**< SHA-256 of archive data   */
     uint32_t num_deps;
     uint32_t num_files;
     uint32_t dep_offset;                /**< Offset to dep array in pkg   */
     uint32_t file_offset;               /**< Offset to file list          */
     uint32_t data_offset;               /**< Offset to compressed payload */
     uint8_t  _pad[24];                  /**< Pad to 512-byte alignment    */
 } uiox_pkg_hdr_t;
 
 /* =========================================================================
  * Package state (in-memory registry)
  * ====================================================================== */
 
 typedef enum {
     UIOX_PKG_STATE_NONE       = 0,
     UIOX_PKG_STATE_AVAILABLE,     /**< Known but not installed           */
     UIOX_PKG_STATE_INSTALLING,
     UIOX_PKG_STATE_INSTALLED,
     UIOX_PKG_STATE_REMOVING,
     UIOX_PKG_STATE_UPGRADING,
     UIOX_PKG_STATE_ERROR,
 } uiox_pkg_state_t;
 
 /* =========================================================================
  * In-memory package record
  * ====================================================================== */
 
 typedef struct uiox_pkg_rec {
     uiox_pkg_hdr_t       hdr;
     uiox_pkg_dep_t       deps    [UIOX_PKG_MAX_DEPS];
     uiox_pkg_file_t      files   [UIOX_PKG_MAX_FILES];
     uiox_pkg_state_t     state;
     uint32_t             dep_count;
     uint32_t             file_count;
     uint64_t             install_time;  /**< Unix timestamp              */
     uint8_t              in_use;
     struct uiox_pkg_rec *next;          /**< Hash chain                  */
 } uiox_pkg_rec_t;
 
 /* =========================================================================
  * Utility macros
  * ====================================================================== */
 
 #define UIOX_PKG_UNUSED(x)  ((void)(x))
 #define UIOX_PKG_MIN(a,b)   ((a)<(b)?(a):(b))
 #define UIOX_PKG_MAX(a,b)   ((a)>(b)?(a):(b))
 #define UIOX_PKG_ARRAY_SIZE(a) (sizeof(a)/sizeof((a)[0]))
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_PKG_TYPES_H */
 