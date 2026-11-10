# =============================================================================
# 50_UIX/01_shell/Makefile — the shell layer
#
# PATH    50_UIX/01_shell/Makefile
#
# This file's absence is why every build printed
#   [KIX] 50_UIX/01_shell: skipped (no Makefile)
# Nothing under this directory has ever been compiled.
#
# ── USERS APACE, NOT KERNEL ─────────────────────────────────────────────────
# These are PROGRAMS, not kernel objects.  A front-end runs at EL0 and
# reaches the kernel through the 40_SCIX trap layer; no kernel object goes
# on any link line here and no 30_KIX header is included.
#
#     write(1, ...)  →  sys_write()  →  my_syscall3  →  SVC/ecall
#     read (0, ...)  →  sys_read()   →  my_syscall3  →  SVC/ecall
#
# fd 0/1/2 ARE the console — bound at boot by uiox_console_bind() from
# uiox_uart_probe(), and intercepted by descriptor number in
# uiox_kix_router.c ahead of the SCFS table.  So a front-end never opens
# /dev/ttyS0 and never calls a uiox_kix_scfs_*() function.
#
# ── the four modalities ─────────────────────────────────────────────────────
#   01_cli   command line      — the one that has sources
#   02_gui   graphical         — empty
#   03_vbi   voice             — empty
#   04_nli   natural language  — empty
#
# Each is a PEER of the others; none may include another.  A modality with
# no sources needs no Makefile here: the loop below skips it and says so.
# =============================================================================

.PHONY: all frontends cli clean check-layering

all: frontends

# ── the front-ends, in order ───────────────────────────────────────────────
# Each is independent: a failure in one does not stop the next — the same
# rule uiox_dss_init.c follows for its probes ("a machine with no camera
# must still find its keyboard").
#
# No arch is named here because the arch targets live in each front-end's
# own Makefile, exactly as 50_UIX/20_uios/Makefile does it.  This file only
# orders the recursion.
frontends:
	@for d in 01_cli 02_gui 03_vbi 04_nli ; do \
		if [ -f $$d/Makefile ] ; then \
			echo "[SHELL]  $$d" ; \
			$(MAKE) --no-print-directory -C $$d || exit 1 ; \
		else \
			echo "[SHELL]  $$d: skipped (no Makefile)" ; \
		fi ; \
	done

# ── the leak test ──────────────────────────────────────────────────────────
# A front-end may not include a 30_KIX header and may not name a
# uiox_kix_scfs_*() function.  Zero hits is the correct answer; any hit
# means a modality is reaching into the kernel instead of trapping.
#
#   the two names that MUST NOT appear:
#     uiox_kix_scfs_      the kernel's file-level entry points
#     uiox_console_       the console binding, which is below the boundary
#
#   the one that MUST:   40_SCIX/uix_sys.h
.PHONY: check-layering
check-layering:
	@found=0 ; \
	for d in 01_cli 02_gui 03_vbi 04_nli ; do \
		if grep -rn 'uiox_kix_scfs_\|uiox_console_\|uiox_uart' $$d 2>/dev/null ; then \
			found=1 ; \
		fi ; \
	done ; \
	if [ $$found -ne 0 ] ; then \
		echo "[SHELL] LAYERING LEAK: a front-end named a kernel symbol" ; \
		exit 1 ; \
	fi ; \
	echo "[SHELL] layering ok: no kernel symbol in any front-end"

clean:
	@for d in 01_cli 02_gui 03_vbi 04_nli ; do \
		[ -f $$d/Makefile ] && $(MAKE) --no-print-directory -C $$d clean ; \
	done
	@echo "[CLN] 01_shell build outputs removed"

# ── note on the umbrella ───────────────────────────────────────────────────
# 30_KIX/Makefile's 50_UIX line currently reads
#     ... || echo "[KIX] 50_UIX/01_shell: skipped"
# Once this file exists, drop the `|| echo` — a genuine compile failure
# must stop the build, not be reported as "skipped".  That message is what
# made this directory invisible for as long as it has been.
#
# Note also that 30_KIX builds the KERNEL, and these are programs for the
# filesystem — so the umbrella's call here, if kept at all, is a
# convenience for building both in one command, not a dependency.  The
# shell does NOT go into libdev.a and is NOT linked into uiox_kernel.elf.
# ───────────────────────────────────────────────────────────────────────────
