#!/bin/bash
#
# build_uix.sh — the USERSPACE image, all four architectures.
#
#   ./build_uix.sh                       # all four, clean each
#   ./build_uix.sh arm64                 # one arch
#   ./build_uix.sh arm64 arm32           # a subset
#   ./build_uix.sh --no-clean arm64      # build + stage only
#   ./build_uix.sh --continue            # don't stop on the first arch that fails
#   ./build_uix.sh --help
#
# WHY THIS EXISTS SEPARATELY FROM rebuild.sh
#   rebuild.sh builds the KERNEL: FwHal, BSP, DSS, FS, PCS, then the link.
#   This builds what the kernel will exec: uios and the ten apps.
#
#   The two are independent by design.  A userspace build failure must not
#   stop four architectures from linking, and a kernel failure must not
#   stop the userspace staging being inspected.  rebuild.sh runs this
#   script's two steps inline as a non-fatal optional layer; this one runs
#   them alone, so a userspace problem can be iterated on without paying
#   for a full kernel rebuild.
#
# ── what it produces ───────────────────────────────────────────────────
#   per architecture, under each makefile's own build/:
#
#     50_UIX/20_uios/build/$ARCH/image/bin/uios
#     50_UIX/21_apps/build/$ARCH/image/21_apps_/app_*
#
#   Those are the paths kinit_main execs and uios's g_apps[] names.  Plain
#   files, not a volume — getting them onto a device is the UNFS format,
#   a separate step.
#
# ── why `make <arch>` and not `make <arch> install` ────────────────────
#   The arch target does the staging itself (uiox_kix._build_install).
#   `make <arch> install` would run install in the PARENT make, where
#   _ARCH is still its default arm64 — which is how every architecture
#   once installed the arm64 binary into the arm64 tree.
#
#   NOTE THIS IS FOR A FREESTANDING CROSS BUILD.  The makefiles default
#   CC to the host `cc` and take ARCH_CFLAGS from the environment, so the
#   flags below are what make the output a target binary rather than a
#   host one.  Pass them always — a hand invocation that omitted them
#   produced host binaries staged as target ones.

set -uo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LOG="${LOG:-$ROOT/build_uix.log}"

# ── per-arch toolchain and flags ─────────────────────────────────────────
# Mirrors rebuild.sh's toolchain table so a user binary is built for the
# same target as the kernel it will run on.
arch_toolchain() {
    case "$1" in
    arm64)
        CC="aarch64-elf-gcc"
        ARCH_CFLAGS="-march=armv8-a -mabi=lp64 -mlittle-endian"
        ARCH_CPUDEF="-D__aarch64__"
        ;;
    arm32)
        CC="arm-none-eabi-gcc"
        ARCH_CFLAGS="-march=armv7-a -marm -mfloat-abi=soft -mlittle-endian"
        ARCH_CPUDEF="-D__arm__"
        ;;
    riscv64)
        CC="riscv64-elf-gcc"
        ARCH_CFLAGS="-march=rv64imafdc_zicsr_zifencei -mabi=lp64d -mcmodel=medany"
        ARCH_CPUDEF="-D__riscv -D__riscv_xlen=64"
        ;;
    x86_64)
        CC="x86_64-elf-gcc"
        ARCH_CFLAGS="-m64 -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -fno-pic -mcmodel=kernel"
        ARCH_CPUDEF="-D__x86_64__"
        ;;
    *)
        return 1
        ;;
    esac
    AR="${CC%-gcc}-ar"
    return 0
}

# ── output helpers ───────────────────────────────────────────────────────
say()  { printf '\n== %s ==\n' "$1"; }
ok()   { printf '\033[32m   ok\033[0m\n'; }
warn() { printf '\033[33m   %s\033[0m\n' "$1"; }
fail() { printf '\033[31m   %s\033[0m\n' "$1"; }

run_make() {
    local label="$1" dir="$2" arch="$3"; shift 3
    printf '   %-34s' "$label"
    if [ ! -f "$dir/Makefile" ]; then
        warn "skipped (no Makefile)"; return 2
    fi
    if ( cd "$dir" && make "$arch" "$@" ) >>"$LOG" 2>&1; then
        ok; return 0
    fi
    fail "FAILED"; return 1
}

# ── one architecture ─────────────────────────────────────────────────────
build_arch() {
    local ARCH="$1" CLEAN="$2"
    local CC AR ARCH_CFLAGS ARCH_CPUDEF

    arch_toolchain "$ARCH" || { fail "unknown arch '$ARCH'"; return 1; }

    if ! command -v "$CC" >/dev/null 2>&1; then
        say "$ARCH"
        printf '   %-34s' "toolchain"
        warn "missing: $CC (not a build failure)"
        return 2
    fi

    say "$ARCH  (CC=$CC)"

    if [ "$CLEAN" = "1" ]; then
        for d in 50_UIX/20_uios 50_UIX/21_apps; do
            [ -f "$ROOT/$d/Makefile" ] && ( cd "$ROOT/$d" && make "clean-$ARCH" ) >>"$LOG" 2>&1
        done
        printf '   %-34s' "clean"
        ok
    fi

    # ── build + stage ──────────────────────────────────────────────────
    # The arch target builds AND installs; see the note at the top on why
    # `install` is not invoked separately.
    run_make "50_UIX/20_uios" "$ROOT/50_UIX/20_uios" "$ARCH" \
        CC="$CC" AR="$AR" \
        ARCH_CFLAGS="$ARCH_CFLAGS" ARCH_CPUDEF="$ARCH_CPUDEF" || return 1

    run_make "50_UIX/21_apps" "$ROOT/50_UIX/21_apps" "$ARCH" \
        CC="$CC" AR="$AR" \
        ARCH_CFLAGS="$ARCH_CFLAGS" ARCH_CPUDEF="$ARCH_CPUDEF" || return 1

    # ── verify what landed ─────────────────────────────────────────────
    # A silent staging failure went unnoticed once for a long time: every
    # architecture wrote into arm64/image and the last one won.  Reporting
    # the size and the count, per arch, is what makes that visible.
    say "image ($ARCH)"
    local IOSBIN="$ROOT/50_UIX/20_uios/build/$ARCH/image/bin/uios"
    local APPSDIR="$ROOT/50_UIX/21_apps/build/$ARCH/image/21_apps_"

    printf '   %-34s' "bin/uios"
    if [ -f "$IOSBIN" ]; then
        printf '%8s bytes\n' "$(wc -c <"$IOSBIN" | tr -d ' ')"
    else
        fail "MISSING"; return 1
    fi

    printf '   %-34s' "21_apps_/"
    if [ -d "$APPSDIR" ]; then
        local n; n="$(ls "$APPSDIR" 2>/dev/null | wc -l | tr -d ' ')"
        if [ "$n" -eq 10 ]; then
            printf '%8s binaries\n' "$n"
        else
            printf '\033[33m%8s binaries — expected 10\033[0m\n' "$n"
        fi
    else
        fail "MISSING"; return 1
    fi

    return 0
}

# ── argument handling ────────────────────────────────────────────────────
CLEAN=1
CONTINUE=0
ARCHES=""

for a in "$@"; do
    case "$a" in
    --no-clean) CLEAN=0 ;;
    --continue) CONTINUE=1 ;;
    --help|-h)
        sed -n '2,40p' "$ROOT/build_uix.sh" | sed 's/^# \{0,1\}//'
        exit 0 ;;
    -*)
        printf 'unknown option: %s\n' "$a" >&2; exit 2 ;;
    *)  ARCHES="$ARCHES $a" ;;
    esac
done

[ -z "$ARCHES" ] && ARCHES=" arm64 arm32 riscv64 x86_64"

: >"$LOG"
printf 'UIOX userspace image — arches:%s\nlog: %s\n' "$ARCHES" "$LOG"
printf 'shell: bash %s\n' "${BASH_VERSION:-?}"

RC=0
SUMMARY=""
for a in $ARCHES; do
    build_arch "$a" "$CLEAN"
    st=$?
    case $st in
    0) SUMMARY="$SUMMARY
   $a  ok" ;;
    2) SUMMARY="$SUMMARY
   $a  no toolchain" ;;
    *) SUMMARY="$SUMMARY
   $a  FAILED"; RC=1 ;;
    esac
    if [ "$st" -ne 0 ] && [ "$st" -ne 2 ] && [ "$CONTINUE" -eq 0 ]; then
        break
    fi
done

say "summary"
printf '%s\n' "$SUMMARY"
printf '   log: %s\n' "$LOG"
exit "$RC"
