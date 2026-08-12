/*
 * 30_KIX/32_FS/10_unfs/unfs_xattr.c
 *
 * UNFS Extended Attributes implementation.
 *
 * Extended attributes store arbitrary name=value pairs on an inode.
 * They are stored in the in-memory inode private data and written
 * back to a dedicated xattr block when the inode is synced.
 *
 * Special well-known xattr names:
 *   "security.mac_label"   — 16-byte MAC label (33_PCS/05_sec)
 *   "security.mac_flags"   — uint32_t MAC flags
 *   "system.immutable"     — 1 byte: 1=immutable, 0=mutable
 *   "user.*"               — application-defined attributes
 *
 * @version 1.0.0  @date 2026-07-29
 */

 #include "unfs_fs.h"
 #include "uiox_soc_string.h"
 #include "uiox_soc_stdio.h"
 
 /* ─────────────────────────────────────────────────────────────────────
  * unfs_xattr_get
  * ───────────────────────────────────────────────────────────────────── */
 int unfs_xattr_get(unfs_inode_priv_t *priv,
                     const char        *name,
                     void              *val_out,
                     uint16_t          *len_out)
 {
     if (!priv || !name) return -22;
 
     uint8_t nlen = 0u;
     while (name[nlen] && nlen < 63u) nlen++;
 
     for (uint8_t i = 0u; i < priv->n_xattrs; i++) {
         unfs_xattr_t *xa = &priv->xattrs[i];
         if (!xa->inuse) continue;
 
         uint8_t xlen = 0u;
         while (xa->name[xlen] && xlen < 63u) xlen++;
         if (xlen != nlen) continue;
 
         uint8_t match = 1u;
         for (uint8_t j = 0u; j < nlen; j++)
             if (xa->name[j] != name[j]) { match = 0u; break; }
 
         if (match) {
             if (val_out && len_out) {
                 uint16_t copy = xa->val_len < *len_out
                               ? xa->val_len : *len_out;
                 memcpy(val_out, xa->value, copy);
                 *len_out = copy;
             }
             return 0;
         }
     }
     return -2;   /* ENOENT — attribute not found */
 }
 
 /* ─────────────────────────────────────────────────────────────────────
  * unfs_xattr_set
  * ───────────────────────────────────────────────────────────────────── */
 int unfs_xattr_set(unfs_inode_priv_t *priv,
                     const char        *name,
                     const void        *val,
                     uint16_t           len)
 {
     if (!priv || !name) return -22;
     if (len > UNFS_XATTR_VAL_MAX) return -22;
 
     uint8_t nlen = 0u;
     while (name[nlen] && nlen < 63u) nlen++;
 
     /* Check if attribute already exists — update in place */
     for (uint8_t i = 0u; i < priv->n_xattrs; i++) {
         unfs_xattr_t *xa = &priv->xattrs[i];
         if (!xa->inuse) continue;
 
         uint8_t xlen = 0u;
         while (xa->name[xlen] && xlen < 63u) xlen++;
         if (xlen != nlen) continue;
 
         uint8_t match = 1u;
         for (uint8_t j = 0u; j < nlen; j++)
             if (xa->name[j] != name[j]) { match = 0u; break; }
 
         if (match) {
             memcpy(xa->value, val, len);
             xa->val_len = len;
             priv->dirty = 1u;
 
             /* Mirror MAC label directly into inode for fast access */
             if (nlen == 19u) {  /* strlen("security.mac_label") */
                 int is_mac = 1;
                 const char *mac = "security.mac_label";
                 for (int k = 0; k < 19; k++)
                     if (name[k] != mac[k]) { is_mac = 0; break; }
                 if (is_mac && len >= 16u)
                     memcpy(priv->disk.i_mac_label, val, 16u);
             }
             return 0;
         }
     }
 
     /* New attribute */
     if (priv->n_xattrs >= UNFS_XATTR_MAX) return -28;  /* ENOSPC */
 
     unfs_xattr_t *xa = &priv->xattrs[priv->n_xattrs];
     memset(xa, 0, sizeof(*xa));
     for (uint8_t j = 0u; j < nlen; j++) xa->name[j] = name[j];
     xa->name[nlen] = '\0';
     memcpy(xa->value, val, len);
     xa->val_len = len;
     xa->inuse   = 1u;
     priv->n_xattrs++;
     priv->dirty = 1u;
     return 0;
 }
 
 /* ─────────────────────────────────────────────────────────────────────
  * unfs_xattr_list — fill buffer with NUL-separated names
  * ───────────────────────────────────────────────────────────────────── */
 int unfs_xattr_list(unfs_inode_priv_t *priv,
                      char              *buf,
                      uint32_t           buf_size)
 {
     if (!priv || !buf || buf_size == 0u) return -22;
 
     uint32_t off = 0u;
     for (uint8_t i = 0u; i < priv->n_xattrs; i++) {
         unfs_xattr_t *xa = &priv->xattrs[i];
         if (!xa->inuse) continue;
 
         uint8_t nlen = 0u;
         while (xa->name[nlen] && nlen < 63u) nlen++;
 
         if (off + nlen + 1u >= buf_size) return -28;  /* ENOSPC */
         memcpy(buf + off, xa->name, nlen);
         buf[off + nlen] = '\0';
         off += nlen + 1u;
     }
     return (int)off;
 }
 