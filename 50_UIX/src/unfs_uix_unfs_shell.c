/*
 * 50_UIX/src/unfs_shell.c
 *
 * UNFS shell utilities — minimal built-in commands for 50_UIX/01_shell.
 *
 * Commands:
 *   unfs-snap-create <mountpoint> <name>   create snapshot
 *   unfs-snap-list   <mountpoint>          list snapshots
 *   unfs-snap-delete <mountpoint> <id>     delete snapshot
 *   unfs-info        <mountpoint>          show filesystem info
 *   unfs-getlabel    <path>                get MAC label
 *   unfs-setlabel    <path> <hex16>        set MAC label
 *   unfs-mkfs        <device> <size_mb>    format device as UNFS
 *   unfs-fsck        <device>              check UNFS volume
 *
 * @version 1.0.0  @date 2026-07-29
 */

 #include "unfs_user.h"
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 
 /* ── unfs-snap-create ───────────────────────────────────────────────── */
 int cmd_unfs_snap_create(int argc, char **argv)
 {
     if (argc < 3) {
         printf("usage: unfs-snap-create <mountpoint> <name>\n");
         return 1;
     }
     int rc = unfs_snapshot_create(argv[1], argv[2]);
     if (rc == 0)
         printf("snapshot '%s' created\n", argv[2]);
     else
         printf("error: %d\n", rc);
     return rc;
 }
 
 /* ── unfs-snap-list ─────────────────────────────────────────────────── */
 int cmd_unfs_snap_list(int argc, char **argv)
 {
     if (argc < 2) {
         printf("usage: unfs-snap-list <mountpoint>\n");
         return 1;
     }
     unfs_snap_info_t snaps[UNFS_SNAP_MAX];
     int n = unfs_snapshot_list(argv[1], snaps, UNFS_SNAP_MAX);
     if (n < 0) { printf("error: %d\n", n); return n; }
     if (n == 0) { printf("no snapshots\n"); return 0; }
 
     printf("%-6s  %-32s  %s\n", "ID", "Name", "Created (ns)");
     printf("------  --------------------------------  --------------------\n");
     for (int i = 0; i < n; i++) {
         printf("%-6llu  %-32s  %llu\n",
                (unsigned long long)snaps[i].snap_id,
                snaps[i].name,
                (unsigned long long)snaps[i].created_ns);
     }
     return 0;
 }
 
 /* ── unfs-snap-delete ───────────────────────────────────────────────── */
 int cmd_unfs_snap_delete(int argc, char **argv)
 {
     if (argc < 3) {
         printf("usage: unfs-snap-delete <mountpoint> <id>\n");
         return 1;
     }
     uint64_t id = (uint64_t)strtoull(argv[2], NULL, 10);
     int rc = unfs_snapshot_delete(argv[1], id);
     if (rc == 0)
         printf("snapshot %llu deleted\n", (unsigned long long)id);
     else
         printf("error: %d\n", rc);
     return rc;
 }
 
 /* ── unfs-info ──────────────────────────────────────────────────────── */
 int cmd_unfs_info(int argc, char **argv)
 {
     if (argc < 2) {
         printf("usage: unfs-info <mountpoint>\n");
         return 1;
     }
     unfs_fsinfo_t info;
     int rc = unfs_fsinfo(argv[1], &info);
     if (rc != 0) { printf("error: %d\n", rc); return rc; }
 
     printf("UNFS Filesystem Info\n");
     printf("  Magic:         0x%08X\n", info.magic);
     printf("  Version:       %u.%u\n",  info.version_major,
                                           info.version_minor);
     printf("  Volume:        %s\n",     info.volume_name);
     printf("  Block size:    %u bytes\n",info.block_size);
     printf("  Total blocks:  %llu\n",
            (unsigned long long)info.block_count);
     printf("  Free blocks:   %llu\n",
            (unsigned long long)info.free_blocks);
     printf("  Total inodes:  %u\n",    info.inode_count);
     printf("  Free inodes:   %u\n",    info.free_inodes);
     printf("  Groups:        %u\n",    info.group_count);
     printf("  COW:           %s\n",    info.cow_enabled ? "on" : "off");
     printf("  Clean:         %s\n",    info.clean ? "yes" : "no");
     return 0;
 }
 
 /* ── unfs-getlabel ──────────────────────────────────────────────────── */
 int cmd_unfs_getlabel(int argc, char **argv)
 {
     if (argc < 2) {
         printf("usage: unfs-getlabel <path>\n");
         return 1;
     }
     uint8_t label[16];
     int rc = unfs_mac_label_get(argv[1], label);
     if (rc != 0) { printf("error: %d\n", rc); return rc; }
 
     printf("MAC label: ");
     for (int i = 0; i < 16; i++)
         printf("%02x", label[i]);
     printf("\n");
     return 0;
 }
 
 /* ── unfs-setlabel ──────────────────────────────────────────────────── */
 int cmd_unfs_setlabel(int argc, char **argv)
 {
     if (argc < 3) {
         printf("usage: unfs-setlabel <path> <hex32>\n");
         printf("  hex32: 32 hex chars = 16 bytes\n");
         return 1;
     }
     const char *hex = argv[2];
     if (strlen(hex) != 32u) {
         printf("error: label must be exactly 32 hex chars\n");
         return 1;
     }
     uint8_t label[16];
     for (int i = 0; i < 16; i++) {
         char byte_str[3] = { hex[i*2], hex[i*2+1], 0 };
         label[i] = (uint8_t)strtoul(byte_str, NULL, 16);
     }
     int rc = unfs_mac_label_set(argv[1], label);
     if (rc == 0)
         printf("label set\n");
     else
         printf("error: %d\n", rc);
     return rc;
 }
 
 /* ── unfs-mkfs ──────────────────────────────────────────────────────── */
 int cmd_unfs_mkfs(int argc, char **argv)
 {
     if (argc < 3) {
         printf("usage: unfs-mkfs <device> <size_mb>\n");
         return 1;
     }
     uint64_t size_mb = (uint64_t)strtoull(argv[2], NULL, 10);
     if (size_mb == 0u) { printf("invalid size\n"); return 1; }
 
     unfs_mkfs_params_t params;
     memset(&params, 0, sizeof(params));
     params.volume_size_bytes = size_mb * 1024u * 1024u;
     params.block_size        = UNFS_BLOCK_SIZE;
     params.inodes_per_group  = 512u;
     params.journal_blocks    = 256u;
     params.enable_cow        = 1u;
     params.enable_checksum   = 1u;
     snprintf((char *)params.volume_name, 64, "unfs");
 
     printf("formatting %s as UNFS (%llu MB)...\n",
            argv[1], (unsigned long long)size_mb);
     int rc = unfs_mkfs(argv[1], &params);
     if (rc == 0)
         printf("done\n");
     else
         printf("error: %d\n", rc);
     return rc;
 }
 
 /* ── unfs-fsck ──────────────────────────────────────────────────────── */
 int cmd_unfs_fsck(int argc, char **argv)
 {
     if (argc < 2) {
         printf("usage: unfs-fsck <device> [--repair]\n");
         return 1;
     }
     int repair = (argc >= 3 && strcmp(argv[2], "--repair") == 0);
     printf("checking %s...\n", argv[1]);
     int rc = unfs_fsck(argv[1], repair);
     if (rc == 0)
         printf("clean\n");
     else
         printf("errors found: %d\n", rc);
     return rc;
 }
 