# UIOX build scripts

Three scripts, two jobs. `rebuildkernel.sh` builds the kernel. `rebuild_uix.sh`
builds the userspace image. `rebuildoverall.sh` does both, kernel first. They are
independent on purpose: a userspace failure must not stop four architectures from
linking, and a kernel failure must not stop the staged binaries being inspected.

```
rebuildkernel.sh   →  02_FwHal, 10_BSP, 30_DSS, 32_FS, 33_PCS, then the link
rebuild_uix.sh     →  50_UIX/20_uios + 50_UIX/21_apps, staged per arch
rebuildoverall.sh  →  both, in that order
```

All three share the same toolchain table, the same `--no-clean` / `--continue`
flags, and the same `arch_toolchain` / `run_make` helpers. Pick by what changed:
kernel sources → `rebuildkernel.sh`; `40_SCIX/uix_sys.h` or an app →
`rebuild_uix.sh`; a cold tree or a full check → `rebuildoverall.sh`.

---

## rebuildkernel.sh — the kernel

```sh
./rebuildkernel.sh                  # all four arches, clean each
./rebuildkernel.sh arm64            # one arch
./rebuildkernel.sh arm64 riscv64    # a subset
./rebuildkernel.sh --no-clean arm64 # rebuild + relink only
./rebuildkernel.sh --continue       # don't stop on the first arch that fails
./rebuildkernel.sh --help
```

**Builds, in order:** `10_BSP` → `30_DSS` (10_DI + 20_DD) → `32_FS` → `33_PCS` →
`02_FwHal` → the kernel link.

**Produces:** `30_KIX/build/<arch>/uiox_kernel.elf` (plus `.map`), and the layer
archives `libbsp.a`, `libdev.a`, `libdi.a`, `libfs.a`, `libpcs.a`,
`libuioxfw<arch>.a`.

**Log:** `build.log`. On failure the last 25 lines are printed.

**Why it exists** rather than `make clean && make arm64`:

- `clean-subsys` deletes `02_FwHal/build` and `30_DSS/10_DI/build`, and nothing
  reliably rebuilds them afterwards
- `_build_subsystems` steps 1/5/6/7 end with `$(_ARCH) 2>/dev/null || echo "skipped"`,
  which reports **success when the step actually failed** — so the umbrella
  finishes with archives missing and the link then fails on
  `uiox_fw_stor_get` / `uiox_proc_init` / `phys_alloc_page`

This script drives each layer directly, passes `ARCH_CFLAGS` / `ARCH_CPUDEF` /
`DEVICE_DEFS` on every call, and stops at the first real failure.

**The flags matter.** A hand-build that omitted `ARCH_CFLAGS`/`ARCH_CPUDEF`
produced objects `aarch64-elf-nm` could not read (`file format not recognized`),
which made later links fail confusingly. Pass them always — the script does.

---

## rebuild_uix.sh — the userspace image

```sh
./rebuild_uix.sh                  # all four arches, clean each
./rebuild_uix.sh arm64            # one arch
./rebuild_uix.sh arm64 riscv64    # a subset
./rebuild_uix.sh --no-clean arm64 # rebuild + stage only
./rebuild_uix.sh --continue       # don't stop on the first arch that fails
./rebuild_uix.sh --help
```

**Builds:** `50_UIX/20_uios` (the launcher) and `50_UIX/21_apps` (the ten apps).

**Produces, per arch:**

```
50_UIX/20_uios/build/<arch>/image/bin/uios
50_UIX/21_apps/build/<arch>/image/21_apps_/app_*
```

Those are the paths `kinit_main` execs and `uios`'s `g_apps[]` names. Plain files,
not a volume — getting them onto a device is the UNFS format, a separate step.

**Log:** `build_uix.log` — deliberately separate, so a userspace failure does not
overwrite the kernel log.

**It also reports, per arch:** the `uios` size, the app count against an expected
ten, and the **trap instruction count** (`svc` / `ecall` / `syscall`). That last
one is the check that the `sys_*` wrappers are not still stubs: a wrapper whose
body returns a constant compiles and links perfectly, so the instruction count in
the disassembly is the only cheap evidence the conversion is real.

**Why `make <arch>` and not `make <arch> install`:** the arch target stages itself
(`_build_install`). Running `install` separately executes it in the *parent* make,
where `_ARCH` is still its default `arm64` — which is how every architecture once
wrote its binaries into the arm64 image tree.

**Why the explicit flags:** the makefiles default `CC` to the host `cc`, so
`ARCH_CFLAGS` / `ARCH_CPUDEF` are what make the output a target binary rather than
a host one.

---

## rebuildoverall.sh — kernel plus userspace

```sh
./rebuildoverall.sh                  # all four arches, clean each
./rebuildoverall.sh arm64            # one arch
./rebuildoverall.sh arm64 riscv64    # a subset
./rebuildoverall.sh --no-clean arm64 # rebuild + relink only
./rebuildoverall.sh --continue       # don't stop on the first arch that fails
./rebuildoverall.sh --help
```

**Builds:** everything `rebuildkernel.sh` builds, then the `50_UIX` userspace
steps, then links the kernel.

**Produces:** the kernel ELF *and* the staged image for each arch.

**Log:** `build.log` — the same file as the kernel script, since this is the
kernel script's sequence with the userspace block included.

**One difference that matters:** the userspace steps here are **non-fatal**. If
`50_UIX/20_uios` fails, the run reports it and continues to the link — because the
kernel does not depend on the image, and a userspace failure must not stop four
architectures from linking. `rebuild_uix.sh` makes the same step fatal, which is
what you want when the image *is* the thing you are working on.

**Use it when:** the tree is cold, or you want one command that shows the whole
state — kernel ELF per arch plus image sizes and app counts.

---

## Choosing between them

| Changed | Run |
|---|---|
| a kernel source (`30_KIX/`, `10_BSP/`, `02_FwHal/`) | `./rebuildkernel.sh` |
| `40_SCIX/uix_sys.h`, `uix_archSysCall.h`, or an app | `./rebuild_uix.sh` |
| both, or a cold tree | `./rebuildoverall.sh` |
| nothing, but you want to see the current state | `./rebuildoverall.sh --no-clean` |

`rebuild_uix.sh` exists mainly so that iterating on `40_SCIX/uix_sys.h` does not
cost a kernel relink each time.

---

## Shared behaviour

**Architectures:** `arm64 arm32 riscv64 x86_64`. Default is all four; name any
subset as arguments.

**Toolchain per arch** — the same table in all three scripts:

| Arch | CC | ARCH_CFLAGS |
|---|---|---|
| arm64 | `aarch64-elf-gcc` | `-march=armv8-a -mabi=lp64 -mlittle-endian` |
| arm32 | `arm-none-eabi-gcc` | `-march=armv7-a -marm -mfloat-abi=soft` |
| riscv64 | `riscv64-elf-gcc` (falls back to `riscv64-unknown-elf-gcc`) | `-march=rv64gc -mabi=lp64d -mcmodel=medany` |
| x86_64 | `x86_64-elf-gcc` | `-m64 -mno-red-zone -mcmodel=kernel` |

**A missing compiler is not a failure.** If `CC` is not installed, that arch is
reported as `-- skipped` and the exit status is still 0 — reporting a missing
cross-compiler as a failed build is misleading.

**`--no-clean`** skips the clean phase and rebuilds in place. Right for iterating
on a `.c`; wrong after a **header** change, where the dependency graph may not
have caught the edit.

**`--continue`** does not stop at the first arch that fails, so one run shows the
state of all four.

**Exit status:** 0 if every attempted arch built (a skipped arch does not count
against it), 1 if any arch failed.

---

## Traps worth knowing

**A stale archive member outlives its source.** `ar` archives are incremental: a
removed `.c` leaves its `.o` in the `.a` until the archive is rebuilt, and the
linker then reads duplicate or dangling definitions from it. After removing a
file from a layer, check:

```sh
for m in $(aarch64-elf-ar t 30_KIX/33_PCS/build/arm64/libpcs.a | grep '^uiox_kix_scpcs_'); do
    [ -f "30_KIX/33_PCS/50_scpcs/src/${m%.o}.c" ] || echo "STALE: $m"
done
```

Any `STALE:` line means the archive needs rebuilding, not another source edit.

**Two definitions of the same function are silent.** A duplicate non-static
function is not a linker error — it takes one and discards the other, so the fix
you made in one file can be the one that was ignored. After a structural change:

```sh
cd 30_KIX/33_PCS/50_scpcs/src && for f in uiox_kix_scpcs_*.c; do \
    fn=$(basename "$f" .c); \
    n=$(grep -l "^int64_t ${fn}(\|^int ${fn}(\|^uiox_int64_t ${fn}(" *.c 2>/dev/null | wc -l); \
    [ "$n" -gt 1 ] && echo "DUPLICATE ($n): $fn"; done
```

Empty output means the layer is clean.

**Read the whole log, not its tail.** Several link failures name all their
undefined symbols at once; `tail` shows only the last of them and hides the set:

```sh
grep -nE 'error:|undefined reference|multiple definition' build.log
```

**`(void)function_name;` is not a no-op.** For a *variable* it silences an unused
warning. For a *function* it takes the function's address, so the linker must
resolve the symbol — which fails if nothing defines it. Use a comment instead
when a call is deliberately disabled.

**`nm` marks a defined variable `B` or `D`, not `T`.** Filtering for ` T ` misses
a global variable entirely. Filter on the name:

```sh
aarch64-elf-nm 30_KIX/build/arm64/uiox_kernel.elf | grep -i 'syscall_dispatch'
```
