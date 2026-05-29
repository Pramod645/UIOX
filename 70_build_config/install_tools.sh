#!/usr/bin/env bash
# =============================================================
# build_config/install_tools.sh
# Installs all UIOX cross-compiler toolchains and QEMU.
# Detects Linux (apt) or macOS (brew) automatically.
# Usage:
#   chmod +x build_config/install_tools.sh
#   ./build_config/install_tools.sh
# =============================================================

set -e

echo "=== UIOX Toolchain Installer ==="

# --- Detect OS -----------------------------------------------
OS="$(uname -s)"

if [ "$OS" = "Linux" ]; then
    echo "[Linux] Using apt package manager..."

    sudo apt-get update -y

    sudo apt-get install -y \
        build-essential \
        gcc \
        binutils \
        make \
        gcc-aarch64-linux-gnu \
        binutils-aarch64-linux-gnu \
        gcc-arm-linux-gnueabihf \
        binutils-arm-linux-gnueabihf \
        qemu-system-arm \
        qemu-system-x86 \
        gdb-multiarch \
        python3 \
        python3-pip

    echo "[Linux] All packages installed."

elif [ "$OS" = "Darwin" ]; then
    echo "[macOS] Using Homebrew..."

    if ! command -v brew &>/dev/null; then
        echo "Homebrew not found. Installing..."
        /bin/bash -c \
          "$(curl -fsSL [raw.githubusercontent.com](https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
    fi

    brew update

    brew install \
        gcc \
        make \
        aarch64-elf-gcc \
        arm-none-eabi-gcc \
        qemu \
        gdb \
        python3

    echo "[macOS] All packages installed."
    echo "Note: On macOS use arm-none-eabi-gcc for ARM32 bare-metal."

else
    echo "Unsupported OS: $OS"
    echo "Please install manually:"
    echo "  - gcc (native x86_64)"
    echo "  - aarch64-linux-gnu-gcc"
    echo "  - arm-linux-gnueabihf-gcc"
    echo "  - qemu-system-arm / qemu-system-x86"
    exit 1
fi

echo ""
echo "=== Toolchain Verification ==="

check_tool() {
    printf "  %-30s" "$1"
    if command -v "$1" &>/dev/null; then
        echo "OK  ($($1 --version 2>&1 | head -1))"
    else
        echo "MISSING"
    fi
}

check_tool gcc
check_tool aarch64-linux-gnu-gcc
check_tool arm-linux-gnueabihf-gcc
check_tool aarch64-elf-gcc
check_tool arm-none-eabi-gcc
check_tool qemu-system-aarch64
check_tool qemu-system-arm
check_tool qemu-system-x86_64
check_tool make
check_tool python3

echo ""
echo "=== Done. Run 'make check_tools' to verify UIOX build. ==="
