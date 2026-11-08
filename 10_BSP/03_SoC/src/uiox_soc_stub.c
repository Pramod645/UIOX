/*
 *  10_BSP/03_SoC/src/uiox_soc_stub.c
 *
 *  Symbols uiox_soc_main.c and uiox_soc_arm64.c reference and no TU defines.
 *
 *  ── why this file exists ────────────────────────────────────────────
 *  uiox_soc_main.c:58 declares uiox_soc_hw_<arch>_register() extern and
 *  calls it (arm64 :468, arm32 :468, riscv64 :453, x86_64 :468).
 *  uiox_soc_arm64.c:143 calls uiox_soc_get_clk(), :151 uiox_soc_get_pm(),
 *  :152 uiox_soc_pm_init().  uiox_soc_main.c:314/:321 call
 *  uiox_soc_init_rk3588()/imx8mp().  uiox_soc.h:235/:240/:245/:252 and
 *  uiox_soc_pm.h:125 DECLARE the accessors and the PM init; none had a
 *  definition anywhere in the tree.
 *
 *  ── the multi-arch rule this file follows ───────────────────────────
 *  The kernel links four arches from ONE source tree, and this file is
 *  compiled into libbsp.a for every one of them.  So a fallback that
 *  names uiox_soc_init_arm64() MUST be inside #if defined(__aarch64__),
 *  or the arm32/riscv64/x86_64 links fail with:
 *
 *      uiox_soc_stub.c:NNN: undefined reference to `uiox_soc_init_arm64'
 *
 *  and the same for the four register hooks.  That was the v1.1.0 bug:
 *  rk3588 and imx8mp called the arm64 backend unconditionally.
 *
 *  ── what is NOT stubbed here ────────────────────────────────────────
 *  _soc_stack_base is NOT in this file and must not be.  It is defined
 *  in every kernel linker script — kernel_arm64.ld:112, kernel_arm32.ld:100,
 *  kernel_riscv64.ld:96, kernel_x86_64.ld:95, all as
 *
 *      _stack_top = .;
 *      _soc_stack_base = _stack_top;
 *
 *  inside the .stack block.  Defining it in C as well would give the
 *  kernel two different stack bases depending on which script won.
 *
 *  ── the honesty rule this file follows ──────────────────────────────
 *  The tree's convention, stated in uiox_uart_if.c and elsewhere: a
 *  layer that cannot do the work says so rather than silently succeeding.
 *  So the init backends do NOT pretend to bring up an RK3588.  They fall
 *  through to the generic backend for their OWN arch, which is what
 *  soc_backend_dispatch() already does for an unrecognised part — so a
 *  board reaching one of these still initialises on the generic path
 *  instead of failing.
 *
 *  @version 1.2.0  @date 2026-11-08
 *           1.2.0 guards rk3588/imx8mp to __aarch64__ and adds the three
 *                 missing uiox_soc_hw_<arch>_register hooks.  1.1.0 added
 *                 uiox_soc_pm_init.  1.0.0 had neither.
 */

 #include "uiox_soc.h"
 #include "uiox_soc_types.h"
 #include "uiox_soc_pm.h"        /* uiox_pm_ctx_t, uiox_soc_pm_init proto */
 
 /* ── the state these accessors hand out ────────────────────────────────
  * File-static, not heap and not stack: the accessors return POINTERS,
  * and uiox_soc.h:237 says get_desc() is valid for the life of the
  * program.  Same reasoning as uiox_uart_probe.c's s_uart_dev and the
  * console's own s_console — the layer owns the storage.
  */
 static uiox_soc_desc_t  s_soc_desc;
 static uiox_clk_ctx_t   s_soc_clk;
 static uiox_pm_ctx_t    s_soc_pm;
 
 /* ═════════════════════════════════════════════════════════════════════
  * The accessors
  *
  * Signatures copied verbatim from uiox_soc.h:235, :240, :245, :252.
  * ═════════════════════════════════════════════════════════════════════ */
 
 const uiox_soc_desc_t *uiox_soc_get_desc(void)
 {
     /* uiox_soc.h:237 — "Returns NULL if uiox_soc_init() has not yet been
      * called."  `initialized` is set true by the arch backend at its end,
      * so this honours the documented contract rather than handing back a
      * half-populated descriptor. */
     return s_soc_desc.initialized ? &s_soc_desc : (const uiox_soc_desc_t *)0;
 }
 
 uiox_clk_ctx_t *uiox_soc_get_clk(void)
 {
     /* Handed out unconditionally: uiox_soc_arm64.c:143 passes the result
      * straight into uiox_soc_clk_init(), which fills it.  Returning NULL
      * would make that call a null dereference, not a graceful failure. */
     return &s_soc_clk;
 }
 
 uiox_pm_ctx_t *uiox_soc_get_pm(void)
 {
     /* Same contract: uiox_soc_arm64.c:151 → uiox_soc_pm_init(pm, desc). */
     return &s_soc_pm;
 }
 
 /* ═════════════════════════════════════════════════════════════════════
  * uiox_soc_pm_init — uiox_soc_pm.h:125
  *
  *     uiox_soc_err_t uiox_soc_pm_init(uiox_pm_ctx_t        *ctx,
  *                                     const uiox_soc_desc_t *soc);
  *
  * uiox_pm_ctx_t (uiox_soc_pm.h:115-119) is three members and no more:
  *
  *     uiox_bool_t pd_on      [UIOX_SOC_PD__COUNT];
  *     uiox_bool_t rst_asserted[UIOX_SOC_RST__COUNT];
  *     uiox_bool_t initialized;
  *
  * There is NO member for the descriptor, so `soc` is not stored.  The
  * v1.1.0 body wrote `ctx->soc = soc;` and failed to compile — the field
  * does not exist.  (void) is the honest statement: a written backend
  * would walk UIOX_SOC_PD__COUNT domains and enable rails; this one does
  * not, and does not claim to.
  * ═════════════════════════════════════════════════════════════════════ */
 
 uiox_soc_err_t uiox_soc_pm_init(uiox_pm_ctx_t *ctx,
                                 const uiox_soc_desc_t *soc)
 {
     if (!ctx) return UIOX_SOC_ERR_INVAL;
 
     /* Zero the whole context without memset — this file is built with
      * -nostdinc, and a byte loop keeps the dependency surface to the
      * headers included above.  Same shape as uiox_soc_power.c:13. */
     {
         unsigned char *p = (unsigned char *)ctx;
         for (unsigned int i = 0u; i < sizeof(*ctx); i++) p[i] = 0u;
     }
 
     (void)soc;                 /* no member to store it in — see above */
 
     ctx->initialized = 1u;     /* usable, if empty */
     return UIOX_SOC_OK;
 }
 
 /* ═════════════════════════════════════════════════════════════════════
  * uiox_soc_print — the one-page summary uiox_soc.h:249-253 describes
  *
  * uiox_soc_printf() IS defined (uiox_soc_main.c:229) and is the file's
  * own output path, so this uses it rather than early_puts().  The
  * header's promise is: name, ISA, platform, clock table, memory map,
  * IRQ count, power state, HAL version.  Only what s_soc_desc actually
  * holds is printed — a stub that printed a fictional clock table would
  * be worse than one that printed four lines.
  * ═════════════════════════════════════════════════════════════════════ */
 
 void uiox_soc_print(void)
 {
     if (!s_soc_desc.initialized) {
         uiox_soc_printf("[SOC] no descriptor — uiox_soc_init() not called\n");
         return;
     }
 
     uiox_soc_printf("[SOC] name    : %s\n", s_soc_desc.name);
     uiox_soc_printf("[SOC] cpus    : %u   freq=%u kHz\n",
                     s_soc_desc.num_cpus, s_soc_desc.cpu_freq_khz);
     uiox_soc_printf("[SOC] dram    : 0x%llx + %llu MB\n",
                     (unsigned long long)s_soc_desc.dram_base,
                     (unsigned long long)(s_soc_desc.dram_size >> 20));
     uiox_soc_printf("[SOC] caps    : 0x%08x\n", s_soc_desc.capabilities);
 }
 
 /* ═════════════════════════════════════════════════════════════════════
  * The dispatch backends that were never written
  *
  * Declared extern in uiox_soc_main.c:159-162, all with the same shape,
  * which uiox_soc_types.h:144 confirms is the house convention:
  *
  *     typedef int (*uiox_soc_init_fn_t)(uiox_soc_desc_t *desc);
  *
  * soc_backend_dispatch() reaches rk3588 when a Cortex-A76's CRU probe at
  * 0xFD7C0000 looks live, and imx8mp when a Cortex-A53's CCM probe at
  * 0x30380000 does.  Neither backend exists.
  *
  * ── the per-arch fallback, and why it must be guarded ───────────────
  * rk3588 and imx8mp are ARM64 parts, so their fallback is the arm64
  * backend.  But this file compiles into libbsp.a for ALL FOUR arches:
  * without the __aarch64__ guard the arm32/riscv64/x86_64 links fail on
  * an arm64-only symbol.  Off-arm64 they return UIOX_SOC_ERR_UNSUP —
  * which is accurate, because those parts do not exist on those ISAs and
  * soc_backend_dispatch() never routes to them there anyway.
  * ═════════════════════════════════════════════════════════════════════ */
 
 int uiox_soc_init_rk3588(uiox_soc_desc_t *desc)
 {
 #if defined(__aarch64__)
     uiox_soc_printf("[SOC] rk3588 backend not implemented"
                     " — generic arm64 path\n");
     return uiox_soc_init_arm64(desc);
 #else
     (void)desc;
     return UIOX_SOC_ERR_UNSUP;
 #endif
 }
 
 int uiox_soc_init_imx8mp(uiox_soc_desc_t *desc)
 {
 #if defined(__aarch64__)
     uiox_soc_printf("[SOC] imx8mp backend not implemented"
                     " — generic arm64 path\n");
     return uiox_soc_init_arm64(desc);
 #else
     (void)desc;
     return UIOX_SOC_ERR_UNSUP;
 #endif
 }
 
 int uiox_soc_init_omap4430(uiox_soc_desc_t *desc)
 {
     /* arm32's dispatch sibling of the two above, and its fallback is the
      * arm32 backend for the same reason. */
 #if defined(__arm__)
     uiox_soc_printf("[SOC] omap4430 backend not implemented"
                     " — generic arm32 path\n");
     return uiox_soc_init_arm32(desc);
 #else
     (void)desc;
     return UIOX_SOC_ERR_UNSUP;
 #endif
 }
 
 int uiox_soc_init_th1520(uiox_soc_desc_t *desc)
 {
     /* riscv64's.  Same shape, same reasoning. */
 #if defined(__riscv)
     uiox_soc_printf("[SOC] th1520 backend not implemented"
                     " — generic riscv64 path\n");
     return uiox_soc_init_riscv64(desc);
 #else
     (void)desc;
     return UIOX_SOC_ERR_UNSUP;
 #endif
 }
 
 /* ═════════════════════════════════════════════════════════════════════
  * The four HAL registration hooks
  *
  * uiox_soc_main.c declares and invokes one per arch — arm64 :58/:468,
  * arm32, riscv64 :453, x86_64 :468 — through a macro, and nothing in
  * 10_BSP or 02_FwHal defines any of them.  Nothing reads a registry they
  * could populate either: uiox_soc_hw.h declares the vtable but the tree
  * has no table a backend could be registered into.
  *
  * So each is a genuine no-op, and this is NOT the same kind of stub as
  * the backends above: there is nothing to fall through TO.  Deliberately
  * silent — each runs once during bring-up and a line here would be noise
  * on every boot.
  *
  * All four are present, each in its own guard, because the common code
  * calls the one matching the arch it was compiled for.  v1.1.0 defined
  * only the arm64 hook, which is why arm32/riscv64/x86_64 failed with:
  *
  *     uiox_soc_main.c:468: undefined reference to `uiox_soc_hw_arm32_register'
  * ═════════════════════════════════════════════════════════════════════ */
 
 #if defined(__aarch64__)
 void uiox_soc_hw_arm64_register(void)
 {
     /* No registry to populate.  If that changes, the header it belongs to
      * is uiox_soc_hw.h, and that is where the call would land. */
 }
 #endif
 
 #if defined(__arm__)
 void uiox_soc_hw_arm32_register(void)
 {
     /* as above — see the banner on this group */
 }
 #endif
 
 #if defined(__riscv)
 void uiox_soc_hw_riscv64_register(void)
 {
     /* as above — see the banner on this group */
 }
 #endif
 
 #if defined(__x86_64__)
 void uiox_soc_hw_x86_register(void)
 {
     /* as above — see the banner on this group */
 }
 #endif
 
 /* ═════════════════════════════════════════════════════════════════════
  * REMOVE THIS FILE WHEN
  *
  *   uiox_soc_get_desc / _get_clk / _get_pm  — when the SoC layer keeps
  *       its own contexts instead of handing out file-statics.
  *   uiox_soc_pm_init                        — when the PM backend lands.
  *   uiox_soc_print                          — when uiox_soc_main.c grows
  *       the real one-page summary uiox_soc.h:249-253 promises.
  *   uiox_soc_init_rk3588 / imx8mp / omap4430 / th1520 — when the four
  *       SoC backends are written.  Each is a real bring-up: RK3588 CRU
  *       and PMU, IMX8MP CCM, OMAP4430 PRCM, TH1520 CLINT.
  *   uiox_soc_hw_{arm64,arm32,riscv64,x86}_register — when uiox_soc_hw.h
  *       gains the registration table its vtable implies.
  *
  * Nothing else should ever be added here.  A stub file that grows is a
  * layer that is not being finished.
  * ═════════════════════════════════════════════════════════════════════ */
 