#!/usr/bin/env bash
# Build the kernel and run it in QEMU (raspi4b).
#
# Meant for Git Bash on Windows (also what Claude Code uses as its shell there).
#
# Usage:
#   tools/run.sh [mode] [options]
#
# Modes:
#   run     (default) headless, UART on stdout, stops after a timeout.
#           Use this for automated test runs (e.g. from Claude Code).
#   gui     QEMU window + UART on stdout, no timeout. Quit with Ctrl-A X.
#   debug   like gui, but QEMU waits for GDB on localhost:1234 (-s -S).
#           Attach with "Attach to QEMU (AArch64)" in VS Code.
#
# Options:
#   -t, --timeout SECONDS   timeout for "run" mode (default 10, 0 = none)
#   -n, --no-build          skip the build step
#   -h, --help              show this help
#
# The build only builds the "kernel" target (kernel.elf + kernel8.img),
# so Doxygen docs are not regenerated and the CMake cache is not modified.

set -euo pipefail

PRESET="rpi4-debug"
MODE="run"
TIMEOUT=10
BUILD=1

usage() { sed -n '2,23p' "$0" | sed 's/^# \{0,1\}//'; }

while [[ $# -gt 0 ]]; do
    case "$1" in
        run|gui|debug) MODE="$1" ;;
        -t|--timeout)  TIMEOUT="${2:?missing value for $1}"; shift ;;
        -n|--no-build) BUILD=0 ;;
        -h|--help)     usage; exit 0 ;;
        *) echo "run.sh: unknown argument '$1'" >&2; usage >&2; exit 2 ;;
    esac
    shift
done

# Always work from the repository root, wherever the script is called from.
cd "$(dirname "${BASH_SOURCE[0]}")/.."

BUILD_DIR="build/${PRESET}"
KERNEL_IMG="${BUILD_DIR}/kernel8.img"
DTB="src/platform/rpi4/bcm2711-rpi-4-b.dtb"

# --- Build --------------------------------------------------------------------
if [[ $BUILD -eq 1 ]]; then
    if [[ ! -f "${BUILD_DIR}/build.ninja" ]]; then
        echo "==> Configuring (${PRESET})"
        cmake --preset "${PRESET}"
    fi
    echo "==> Building kernel (${PRESET})"
    cmake --build --preset "${PRESET}" --target kernel
fi

if [[ ! -f "${KERNEL_IMG}" ]]; then
    echo "run.sh: ${KERNEL_IMG} not found (did the build fail, or is llvm-objcopy missing?)" >&2
    exit 1
fi

# --- Find QEMU ----------------------------------------------------------------
QEMU="$(command -v qemu-system-aarch64 || true)"
if [[ -z "${QEMU}" && -x "/c/Program Files/qemu/qemu-system-aarch64.exe" ]]; then
    QEMU="/c/Program Files/qemu/qemu-system-aarch64.exe"
fi
if [[ -z "${QEMU}" ]]; then
    echo "run.sh: qemu-system-aarch64 not found (add C:\\Program Files\\qemu to PATH)" >&2
    exit 1
fi

# --- Run ----------------------------------------------------------------------
QEMU_ARGS=(-M raspi4b -dtb "${DTB}" -kernel "${KERNEL_IMG}" -serial null -serial mon:stdio)

case "${MODE}" in
    run)   QEMU_ARGS+=(-display none) ;;
    gui)   ;;
    debug) QEMU_ARGS+=(-s -S)
           echo "==> QEMU is waiting for GDB on localhost:1234" ;;
esac

echo "==> ${MODE}: $(basename "${QEMU}") ${QEMU_ARGS[*]}"

if [[ "${MODE}" != "run" || "${TIMEOUT}" == "0" ]]; then
    exec "${QEMU}" "${QEMU_ARGS[@]}"
fi

# Headless run with a time limit. The kernel never exits on its own,
# so hitting the timeout is the normal way this ends.
"${QEMU}" "${QEMU_ARGS[@]}" &
QEMU_PID=$!
trap 'kill "${QEMU_PID}" 2>/dev/null || true' EXIT INT TERM

SECONDS=0
while kill -0 "${QEMU_PID}" 2>/dev/null; do
    if (( SECONDS >= TIMEOUT )); then
        kill "${QEMU_PID}" 2>/dev/null || true
        wait "${QEMU_PID}" 2>/dev/null || true
        echo
        echo "==> Stopped QEMU after ${TIMEOUT}s timeout"
        exit 0
    fi
    sleep 0.2
done

# QEMU exited before the timeout: pass its exit code on (a crash shows up here).
set +e
wait "${QEMU_PID}"
STATUS=$?
echo
echo "==> QEMU exited early with status ${STATUS}"
exit "${STATUS}"
