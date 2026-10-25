#!/usr/bin/env bash
#
# rebuild.sh — UIOX kernel, all four architectures.
#
# USAGE
#   ./rebuild.sh                  # all four arches, clean each
#   ./rebuild.sh arm64            # one arch
#   ./rebuild.sh arm64 riscv64    # a subset
#   ./rebuild.sh --no-clean arm64 # rebuild + relink only
#   ./rebuild.sh --continue       # don't stop on the first arch that fails
#   ./rebuild.sh --help
#
# WHY THIS EXISTS
#   `make clean && make arm64` is not reliable on this tree:
#     · clean-subsys deletes 02_FwHal/build and 30_DSS/10_DI/build
#     · _build_subsystems steps 1/5/6/7 end with
#           $(_ARCH) 2>/dev/null || echo "[KIX] …: skipped"
#       which reports success when the step actually failed — so the
#       umbrella finishes with archives missing and the link then fails
#       on uiox_fw_stor_get / uiox_proc_init / phys_alloc_page.
#
#   This script drives each layer directly, passes ARCH_CFLAGS /
#   ARCH_CPUDEF / DEVICE_DEFS on every call, and stops at the first
#   real failure.
#
#   Hand-builds that omitted ARCH_CFLAGS/ARCH_CPUDEF produced objects
#   `aarch64-elf-nm` could not read ("file format not recognized"),
#   which made later links fail confusingly.  Pass them always.

set -uo pipefail

# ── layout ───────────────────────────────────────────────────────────────
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LOG="${LOG:-$ROOT/build.log}"

# ── per-arch toolchain and flags ─────────────────────────────────────────
# arch_toolchain <arch> sets: CC AR LD AS OBJCOPY STRIP
#                            ARCH_CFLAGS ARCH_ASFLAGS ARCH_CPUDEF
#                            ARCH_LD_EMUL ENABLE_ARCH_DEF
arch_toolchain() {
    case "$1" in
        arm64)
            CC=aarch64-elf-gcc;  AR=aarch64-elf-ar;  LD=aarch64-elf-ld
            AS=aarch64-elf-gcc;  OBJCOPY=aarch64-elf-objcopy
            STRIP=aarch64-elf-strip
            ARCH_CFLAGS="-march=armv8-a -mabi=lp64 -mlittle-endian"
            ARCH_ASFLAGS="-march=armv8-a"
            ARCH_CPUDEF="-D__aarch64__"
            ARCH_LD_EMUL="aarch64elf"
            ;;
        arm32)
            CC=arm-none-eabi-gcc;  AR=arm-none-eabi-ar;  LD=arm-none-eabi-ld
            AS=arm-none-eabi-gcc;  OBJCOPY=arm-none-eabi-objcopy
            STRIP=arm-none-eabi-strip
            ARCH_CFLAGS="-march=armv7-a -marm -mfloat-abi=soft"
            ARCH_ASFLAGS="-march=armv7-a"
            ARCH_CPUDEF="-D__arm__"
            ARCH_LD_EMUL="armelf"
            ;;
        riscv64)
            # Match 30_KIX/Makefile (RV64_* := riscv64-elf-*).  Homebrew's
            # riscv64-elf-gcc formula installs WITHOUT the "unknown-" vendor
            # infix.  Prefer the Makefile's name; fall back to the
            # vendor-infix spelling so either install works.
            if command -v riscv64-elf-gcc >/dev/null 2>&1; then
                CC=riscv64-elf-gcc; AR=riscv64-elf-ar
                LD=riscv64-elf-ld;  AS=riscv64-elf-gcc
                OBJCOPY=riscv64-elf-objcopy
                STRIP=riscv64-elf-strip
            else
                CC=riscv64-unknown-elf-gcc; AR=riscv64-unknown-elf-ar
                LD=riscv64-unknown-elf-ld;  AS=riscv64-unknown-elf-gcc
                OBJCOPY=riscv64-unknown-elf-objcopy
                STRIP=riscv64-unknown-elf-strip
            fi
            ARCH_CFLAGS="-march=rv64gc -mabi=lp64d -mcmodel=medany"
            ARCH_ASFLAGS="-march=rv64gc"
            ARCH_CPUDEF="-D__riscv -D__riscv_xlen=64"
            ARCH_LD_EMUL="elf64lriscv"
            ;;
        x86_64)
            CC=x86_64-elf-gcc;  AR=x86_64-elf-ar;  LD=x86_64-elf-ld
            AS=x86_64-elf-gcc;  OBJCOPY=x86_64-elf-objcopy
            STRIP=x86_64-elf-strip
            ARCH_CFLAGS="-m64 -mno-red-zone -mcmodel=kernel"
            ARCH_ASFLAGS="-m64"
            ARCH_CPUDEF="-D__x86_64__"
            ARCH_LD_EMUL="elf_x86_64"
            ;;
        *)
            return 1
            ;;
    esac
    export CC AR LD AS OBJCOPY STRIP
    export ARCH_CFLAGS ARCH_ASFLAGS ARCH_CPUDEF ARCH_LD_EMUL
}

ALL_ARCHES=(arm64 arm32 riscv64 x86_64)

# ── device flags — the full set, same as the umbrella ────────────────────
DEVICE_DEFS="\
-DUIOX_HAVE_WIFI -DUIOX_HAVE_BT -DUIOX_HAVE_ETH -DUIOX_HAVE_USB \
-DUIOX_HAVE_HDMI -DUIOX_HAVE_TB4 -DUIOX_HAVE_MONITOR -DUIOX_HAVE_GPU \
-DUIOX_HAVE_CAMERA -DUIOX_HAVE_MIC -DUIOX_HAVE_KBD -DUIOX_HAVE_MOUSE \
-DUIOX_HAVE_ALS -DUIOX_HAVE_THERMAL -DUIOX_HAVE_TOUCHPWD -DUIOX_HAVE_BMS \
-DUIOX_HAVE_CHG -DUIOX_HAVE_FAN -DUIOX_HAVE_PMIC -DUIOX_HAVE_RTC \
-DUIOX_HAVE_SPEAKER -DUIOX_HAVE_EMMC -DUIOX_HAVE_UART -DUIOX_HAVE_SDMC \
-DUIOX_HAVE_NVME"
export DEVICE_DEFS

ENABLE_FLAGS="\
ENABLE_WIFI=yes ENABLE_BT=yes ENABLE_ETH=yes ENABLE_USB=yes \
ENABLE_HDMI=yes ENABLE_TB4=yes ENABLE_MONITOR=yes ENABLE_GPU=yes \
ENABLE_CAMERA=yes ENABLE_MIC=yes ENABLE_KBD=yes ENABLE_MOUSE=yes \
ENABLE_ALS=yes ENABLE_THERMAL=yes ENABLE_TOUCHPWD=yes ENABLE_BMS=yes \
ENABLE_CHG=yes ENABLE_FAN=yes ENABLE_PMIC=yes ENABLE_RTC=yes \
ENABLE_SPEAKER=yes ENABLE_EMMC=yes ENABLE_UART=yes ENABLE_SDMC=yes \
ENABLE_NVME=yes"
export ENABLE_FLAGS

# ── helpers ──────────────────────────────────────────────────────────────
say() { printf '\n\033[1m== %s\033[0m\n' "$*"; }
die() { printf '\n\033[31m!! %s\033[0m\n' "$*" >&2; exit 1; }
ok()  { printf '   \033[32mok\033[0m\n'; }

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
        --help|-h)  sed -n '2,35p' "$0"; exit 0 ;;
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
    # "skipped", not "FAILED" — riscv64 hits this on a Mac without
    # riscv64-unknown-elf-gcc, and reporting a missing compiler as a failed
    # build is misleading.
    command -v "$CC" >/dev/null || {
        printf -- '-- %s: %s not installed — skipped\n' "$ARCH" "$CC"
        return 2
    }

    command -v "$LD" >/dev/null || {
        printf '\n\033[33m-- %s: %s not installed — skipped\033[0m\n' "$ARCH" "$LD"
        return 2
    }

    printf '\n\033[1m#### %s  (CC=%s)\033[0m\n' "$ARCH" "$CC"

    # 0. clean — kernel objects only.  Plain `make clean` also deletes
    #    02_FwHal/build, and nothing reliably rebuilds it afterwards.
    if [ "$DO_CLEAN" -eq 1 ]; then
        say "clean ($ARCH)"
        run_make "clean-arm64-equivalent" "$ROOT/30_KIX" "clean-$ARCH"
    fi

    # 1. firmware HAL
    say "subsystems ($ARCH)"
    run_make "02_FwHal" "$ROOT/02_FwHal" "$ARCH" \
        CC="$CC" AR="$AR" \
        ARCH_CFLAGS="$ARCH_CFLAGS" ARCH_CPUDEF="$ARCH_CPUDEF" || return 1

    run_make "10_BSP" "$ROOT/10_BSP" "$ARCH" \
        CC="$CC" AR="$AR" AS="$AS" \
        ARCH_CFLAGS="$ARCH_CFLAGS" ARCH_ASFLAGS="$ARCH_ASFLAGS" \
        ARCH_CPUDEF="$ARCH_CPUDEF" LD_EMUL="$ARCH_LD_EMUL" BUILD=static || return 1

    run_make "30_DSS (10_DI + 20_DD)" "$ROOT/30_KIX/30_DSS" "$ARCH" \
        CC="$CC" AR="$AR" \
        ARCH_CFLAGS="$ARCH_CFLAGS" ARCH_CPUDEF="$ARCH_CPUDEF" \
        DEVICE_DEFS="$DEVICE_DEFS" $ENABLE_FLAGS || return 1

    run_make "32_FS" "$ROOT/30_KIX/32_FS" "$ARCH" \
        CC="$CC" AR="$AR" \
        ARCH_CFLAGS="$ARCH_CFLAGS" ARCH_CPUDEF="$ARCH_CPUDEF" || return 1

    run_make "33_PCS" "$ROOT/30_KIX/33_PCS" "$ARCH" \
        CC="$CC" AR="$AR" \
        ARCH_CFLAGS="$ARCH_CFLAGS" ARCH_CPUDEF="$ARCH_CPUDEF" || return 1

    # optional: no .c files in 50_UIX/01_shell yet
    say "optional layers ($ARCH)"
    printf '   %-34s' "50_UIX/01_shell"
    if [ -f "$ROOT/50_UIX/01_shell/Makefile" ]; then
        ( cd "$ROOT/50_UIX/01_shell" && make "$ARCH" CC="$CC" AR="$AR" \
            ARCH_CFLAGS="$ARCH_CFLAGS" ARCH_CPUDEF="$ARCH_CPUDEF" ) >>"$LOG" 2>&1 \
            && ok || printf 'skipped (no sources yet)\n'
    else
        printf 'skipped (no Makefile)\n'
    fi

    # 2. verify every archive the link needs exists
    say "archives ($ARCH)"
    local MISSING=0 a
    for a in "$ROOT/02_FwHal/build/$ARCH/libuiox_fw_$ARCH.a" \
             "$ROOT/10_BSP/build/$ARCH/libbsp.a" \
             "$ROOT/30_KIX/30_DSS/build/$ARCH/libdi.a" \
             "$ROOT/30_KIX/30_DSS/build/$ARCH/libdev.a" \
             "$ROOT/30_KIX/32_FS/build/$ARCH/libfs.a" \
             "$ROOT/30_KIX/33_PCS/build/$ARCH/libpcs.a"; do
        if [ -f "$a" ]; then
            printf '   %8s  %s\n' "$(wc -c <"$a" | tr -d ' ')" "${a#"$ROOT/"}"
        else
            printf '\033[31m   MISSING   %s\033[0m\n' "${a#"$ROOT/"}"
            MISSING=1
        fi
    done
    [ "$MISSING" -eq 0 ] || {
        printf '\033[31m   archives missing — the link would fail on undefined symbols\033[0m\n'
        return 1
    }

    # 3. link
    say "link ($ARCH)"
    run_make "kernel ELF" "$ROOT/30_KIX" "$ARCH" \
        CC="$CC" AR="$AR" LD="$LD" \
        ARCH_CFLAGS="$ARCH_CFLAGS" ARCH_CPUDEF="$ARCH_CPUDEF" \
        ARCH_LD_EMUL="$ARCH_LD_EMUL" \
        DEVICE_DEFS="$DEVICE_DEFS" LINK=yes || return 1

    # 4. verify the ELF
    say "result ($ARCH)"
    local ELF="$ROOT/30_KIX/build/$ARCH/uiox_kernel.elf"
    [ -f "$ELF" ] || { printf '\033[31m   no ELF at %s\033[0m\n' "$ELF"; return 1; }

    local NM="${CC%-gcc}-nm"
    if command -v "$NM" >/dev/null; then
        local UNDEF
        UNDEF="$("$NM" -u "$ELF" 2>/dev/null | head -20)"
        if [ -n "$UNDEF" ]; then
            printf '\033[31m   undefined symbols remain:\033[0m\n%s\n' "$UNDEF" >&2
            return 1
        fi
        printf '   no undefined symbols\n'
    fi

        # Prefer a size tool matching CC, but fall back to a known-good name:
    # arm-none-eabi-gcc -> arm-none-eabi-size works, yet string-mangling
    # silently yields a non-existent tool if CC is ever set to a wrapper
    # (ccache, a versioned binary).  Resolve explicitly, and never let the
    # lookup's status become the function's.
    local SIZE="${CC%-gcc}-size"
    if ! command -v "$SIZE" >/dev/null 2>&1; then
        SIZE="${CC%-gcc}-size"        # try as-is
        command -v "$SIZE" >/dev/null 2>&1 || SIZE="size"
    fi
    if command -v "$SIZE" >/dev/null 2>&1; then
        "$SIZE" "$ELF" | sed 's/^/   /'
        return 0
    fi
    printf '   (no size tool for %s; size not reported)\n' "$CC"
    return 0

    printf '\n   \033[32m%s\033[0m  (%s bytes)\n' "${ELF#"$ROOT/"}" "$(wc -c <"$ELF" | tr -d ' ')"
    return 0
}

# ── run ──────────────────────────────────────────────────────────────────
: >"$LOG"
printf 'UIOX rebuild — arches: %s\nlog: %s\n' "${ARCHES[*]}" "$LOG"

declare -a RESULTS=()
FAILED=0
for A in "${ARCHES[@]}"; do
    rc=0
    build_arch "$A" || rc=$?

    if [ "$rc" -eq 0 ]; then
        RESULTS+=("$A: ok")
    elif [ "$rc" -eq 2 ]; then
        RESULTS+=("$A: skipped (no toolchain)")
    else
        RESULTS+=("$A: FAILED")
        FAILED=1
        if [ "$CONTINUE" -ne 1 ]; then
            printf '\n!! stopping at %s (use --continue to proceed)\n' "$A" >&2
            break
        fi
    fi
done

printf '\n==== summary ====\n'
for r in "${RESULTS[@]}"; do printf '   %s\n' "$r"; done
printf '   log: %s\n' "$LOG"

exit "$FAILED"
