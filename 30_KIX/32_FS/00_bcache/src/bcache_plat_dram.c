/**
 * @file    bcache_plat_dram.c
 * @brief   bcache's DEFAULT platform I/O — a DRAM region, so the
 *          filesystem can be exercised before a driver exists.
 *
 * PATH    30_KIX/32_FS/00_bcache/src/bcache_plat_dram.c
 *
 * This is bcache_init.c's original four hooks, moved out unchanged.  The
 * move is what makes the swap in option B possible: with all four in
 * their own translation unit, 32_FS/Makefile links either this file (the
 * stand-in) or bcache_plat_dev.c (the live path) — never both.
 *
 * Read it from git rather than retyping: the bodies at
 * bcache_init.c:101, :123, :143 and the write hook are exactly what goes
 * here, minus the surrounding buffer-pool initialisation that stays in
 * bcache_init.c.
 *
 * @version 1.0.0  @date 2026-10-16
 */
