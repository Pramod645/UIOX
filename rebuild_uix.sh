#!/usr/bin/env bash
#
# rebuild_uix.sh — UIOX USERSPACE image, all four architectures.
#
# USAGE
#   ./rebuild_uix.sh                  # all four arches, clean each
#   ./rebuild_uix.sh arm64            # one arch
#   ./rebuild_uix.sh arm64 riscv64    # a subset
#   ./rebuild_uix.sh --no-clean arm64 # rebuild + stage only
#   ./rebuild_uix.sh --continue       # don't stop on the first arch that fails
#   ./rebuild_uix.sh --help
#
# WHY THIS IS SEPARATE FROM rebuildkernel.sh
#   rebuildkernel.sh builds the KERNEL: FwHal, BSP, DSS, FS, PCS, then the
#   link.  This builds what the kernel will exec: uios and the ten apps.
#
#   The two are independent on purpose.  A userspace failure must not stop
#   four architectures from linking, and a kernel failure must not stop the
#   staged binaries being inspected.  rebuildkernel.sh runs these two steps
#   inline as a non-fatal optional layer; this script runs them alone, so a
#   userspace problem can be worked on without paying for a kernel rebuild.
#
# WHAT IT PRODUCES
#   per architecture, under each makefile's own build/:
#
#     50_UIX/20_uios/build/$ARCH/image/bin/uios
#     50_UIX/21_apps/build/$ARCH/image/21_apps_/app_*
#
#   Those are the paths kinit_main execs and uios's g_apps[] names.  Plain
#   files, not a volume — getting them onto a device is the UNFS format, a
#   separate step.
#
# WHY `make <arch>` AND NOT `make <arch> install`
#   The arch target does the staging itself (_build_install).  Running
#   `install` separately executes it in the PARENT make, where _ARCH is
#   still its default arm64 — which is how every architecture once wrote
#   its binaries into the arm64 image tree.
#
#   The makefiles default CC to the host `cc`, so the flags below are what
#   make the output a target binary rather than a host one.  Pass them
#   always: a hand invocation that omitted them produced host binaries
#   staged as target ones.

set -uo pipefail

# ── layout ───────────────────────────────────────────────────────────────
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LOG="${LOG:-$ROOT/build_uix.log}"

# ── per-arch toolchain and flags ─────────────────────────────────────────
# arch_toolchain <arch> sets: CC AR ARCH_CFLAGS ARCH_CPUDEF
# Deliberately the same table as rebuildkernel.sh, so a user binary is
# built for the same target as the kernel it will run on.
arch_toolchain() {
    case "$1" in
        arm64)
            CC=aarch64-elf-gcc;  AR=aarch64-elf-ar
            ARCH_CFLAGS="-march=armv8-a -mabi=lp64 -mlittle-endian"
            ARCH_CPUDEF="-D__aarch64__"
            ;;
        arm32)
            CC=arm-none-eabi-gcc;  AR=arm-none-eabi-ar
            ARCH_CFLAGS="-march=armv7-a -marm -mfloat-abi=soft"
            ARCH_CPUDEF="-D__arm__"
            ;;
        riscv64)
            # Same fallback as the kernel script: Homebrew's riscv64-elf-gcc
            # installs without the "unknown-" vendor infix.
            if command -v riscv64-elf-gcc >/dev/null 2>&1; then
                CC=riscv64-elf-gcc; AR=riscv64-elf-ar
            else
                CC=riscv64-unknown-elf-gcc; AR=riscv64-unknown-elf-ar
            fi
            ARCH_CFLAGS="-march=rv64gc -mabi=lp64d -mcmodel=medany"
            ARCH_CPUDEF="-D__riscv -D__riscv_xlen=64"
            ;;
        x86_64)
            CC=x86_64-elf-gcc;  AR=x86_64-elf-ar
            ARCH_CFLAGS="-m64 -mno-red-zone -mcmodel=kernel"
            ARCH_CPUDEF="-D__x86_64__"
            ;;
        *)
            return 1
            ;;
    esac
    export CC AR ARCH_CFLAGS ARCH_CPUDEF
}

ALL_ARCHES=(arm64 arm32 riscv64 x86_64)

# ── helpers ──────────────────────────────────────────────────────────────
say() { printf '\n\033[1m== %s\033[0m\n' "$*"; }
die() { printf '\n\033[31m!! %s\033[0m\n' "$*" >&2; exit 1; }
ok()  { printf '   \033[32mok\033[0m\n'; }
warn(){ printf '   \033[33m%s\033[0m\n' "$*"; }

# run_make <label> <dir> <target...>
# Tail goes to the log always; on failure the last 25 lines are printed.
run_make() {
    local label="$1" dir="$2"; shift 2
    printf '   %-34s' "$label"
    [ -d "$dir" ] || { printf '\033[31mno dir\033[0m\n'; return 1; }

    if ( cd "$dir" && make "$@" ) >>"$LOG" 2>&1; then
        ok
        return 0
    fi
    printf '\033[31mFAILED\033[0m\n'
    printf -- '--- last 25 lines of %s ---\n' "$LOG" >&2
    tail -25 "$LOG" >&2
    return 1
}

# ── argument handling ────────────────────────────────────────────────────
DO_CLEAN=1
CONTINUE=0
ARCHES=()

while [ $# -gt 0 ]; do
    case "$1" in
        --no-clean) DO_CLEAN=0 ;;
        --continue) CONTINUE=1 ;;
        --help|-h)  sed -n '2,45p' "$0"; exit 0 ;;
        -*)         die "unknown option: $1" ;;
        *)          ARCHES+=("$1") ;;
    esac
    shift
done

if [ ${#ARCHES[@]} -eq 0 ]; then
    ARCHES=("${ALL_ARCHES[@]}")
fi

# ── per-arch build ───────────────────────────────────────────────────────
build_arch() {
    local ARCH="$1"
    arch_toolchain "$ARCH" || die "unknown arch: $ARCH (arm64 arm32 riscv64 x86_64)"

    # rc 2 = no toolchain (not a build failure).  The summary reports it as
    # "skipped" rather than FAILED — reporting a missing compiler as a
    # failed build is misleading.
    command -v "$CC" >/dev/null || {
        printf '\n\033[33m-- %s: %s not installed — skipped\033[0m\n' "$ARCH" "$CC"
        return 2
    }

    printf '\n\033[1m#### %s  (CC=%s)\033[0m\n' "$ARCH" "$CC"

    # 0. clean — this arch's objects and staged image only.  A plain
    #    `make clean` in these trees removes every arch's build/.
    if [ "$DO_CLEAN" -eq 1 ]; then
        say "clean ($ARCH)"
        run_make "clean-$ARCH" "$ROOT/50_UIX/20_uios" "clean-$ARCH" || true
        run_make "clean-$ARCH" "$ROOT/50_UIX/21_apps" "clean-$ARCH" || true
    fi

    # 1. uios — the launcher.  The arch target builds AND stages.
    say "userspace ($ARCH)"
    run_make "50_UIX/20_uios" "$ROOT/50_UIX/20_uios" "$ARCH" \
        CC="$CC" AR="$AR" \
        ARCH_CFLAGS="$ARCH_CFLAGS" ARCH_CPUDEF="$ARCH_CPUDEF" || return 1

    run_make "50_UIX/21_apps" "$ROOT/50_UIX/21_apps" "$ARCH" \
        CC="$CC" AR="$AR" \
        ARCH_CFLAGS="$ARCH_CFLAGS" ARCH_CPUDEF="$ARCH_CPUDEF" || return 1

    # 2. verify what landed.  A silent staging failure went unnoticed once
    #    for a long time: every architecture wrote into arm64/image and the
    #    last one won.  Reporting size and count per arch is what makes
    #    that visible.
    say "image ($ARCH)"
    local IOSBIN="$ROOT/50_UIX/20_uios/build/$ARCH/image/bin/uios"
    local APPSDIR="$ROOT/50_UIX/21_apps/build/$ARCH/image/21_apps_"

    printf '   %-34s' "bin/uios"
    if [ -f "$IOSBIN" ]; then
        printf '%8s bytes\n' "$(wc -c <"$IOSBIN" | tr -d ' ')"
    else
        printf '\033[31mMISSING\033[0m\n'
        return 1
    fi

    printf '   %-34s' "21_apps_/"
    if [ -d "$APPSDIR" ]; then
        local n
        n="$(ls "$APPSDIR" 2>/dev/null | wc -l | tr -d ' ')"
        if [ "$n" -eq 10 ]; then
            printf '%8s binaries\n' "$n"
        else
            printf '\033[33m%8s binaries — expected 10\033[0m\n' "$n"
        fi
    else
        printf '\033[31mMISSING\033[0m\n'
        return 1
    fi

    # 3. the trap must be in the binary.  A wrapper whose body still
    #    returns a constant compiles and links fine, so the instruction
    #    count is the check that the conversion is real.
    local OD PAT CNT
    case "$ARCH" in
        arm64)   OD=aarch64-elf-objdump;  PAT='\bsvc\b' ;;
        arm32)   OD=arm-none-eabi-objdump; PAT='\bsvc\b' ;;
        riscv64) OD="${CC%-gcc}-objdump";  PAT='\becall\b' ;;
        x86_64)  OD=x86_64-elf-objdump;    PAT='\bsyscall\b' ;;
    esac
    printf '   %-34s' "trap instructions"
    if command -v "$OD" >/dev/null 2>&1; then
        CNT="$("$OD" -d "$IOSBIN" 2>/dev/null | grep -cE "$PAT")"
        if [ "$CNT" -gt 0 ]; then
            printf '%8s %s\n' "$CNT" "${PAT//\\b/}"
        else
            printf '\033[33m%8s — wrappers may still be stubs\033[0m\n' "$CNT"
        fi
    else
        printf '%8s\n' "no $OD"
    fi

    return 0
}

# ── main ─────────────────────────────────────────────────────────────────
: >"$LOG"
printf 'UIOX userspace image — arches: %s\nlog: %s\n' "${ARCHES[*]}" "$LOG"

RC=0
for ARCH in "${ARCHES[@]}"; do
    build_arch "$ARCH"
    st=$?
    case $st in
        0) ;;                                     # built
        2) ;;                                     # no toolchain
        *) RC=1; [ "$CONTINUE" -eq 0 ] && break ;; # failed
    esac
done

say "summary"
for ARCH in "${ARCHES[@]}"; do
    printf '   %-9s ' "$ARCH"
    BIN="$ROOT/50_UIX/20_uios/build/$ARCH/image/bin/uios"
    if [ -f "$BIN" ]; then
        printf '%s bytes\n' "$(wc -c <"$BIN" | tr -d ' ')"
    else
        printf -- '-- not built\n'
    fi
done
printf '   log: %s\n' "$LOG"
exit "$RC"
