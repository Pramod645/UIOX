/*
 * 50_UIX/01_shell/01_cli/src/uiox_sh.h
 *
 * The front-end's public surface.  One function: the shell's entry.
 *
 * uiox_sh.c defines uix_sh_main().  Its own main() is guarded by
 * UIOX_SH_NO_MAIN, which 21_apps passes when it links the front-end into
 * the packaged app_shell — so the standalone build in 01_cli still gets
 * an entry of its own and the app does not end up with two.
 */
#ifndef __UIX_SH_H
#define __UIX_SH_H

int uix_sh_main(void);

#endif /* __UIX_SH_H */
