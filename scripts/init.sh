#!/usr/bin/env bash
# Installa (senza sudo) tutto il necessario per buildare: workspace Zephyr,
# virtualenv Python, Zephyr SDK (toolchain ARM + host tools) e gperf.
# Idempotente: i passi gia' fatti vengono saltati.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
. "$SCRIPT_DIR/env.sh"

# Moduli west necessari per nRF52840 (evita di scaricare tutto il manifest)
WEST_MODULES="hal_nordic cmsis cmsis_6 segger mbedtls"
GPERF_VERSION=3.1

for tool in git cmake wget tar make gcc python3; do
    command -v "$tool" >/dev/null || { echo "Manca '$tool': installalo con il package manager." >&2; exit 1; }
done

echo "==> Virtualenv Python in $ZEPHYR_WS/.venv"
mkdir -p "$ZEPHYR_WS"
if [ ! -d "$ZEPHYR_WS/.venv" ]; then
    if command -v uv >/dev/null; then
        uv venv --python 3 "$ZEPHYR_WS/.venv"
    else
        python3 -m venv "$ZEPHYR_WS/.venv"
    fi
fi
. "$ZEPHYR_WS/.venv/bin/activate"

pip_install() {
    if command -v uv >/dev/null; then uv pip install "$@"; else pip install "$@"; fi
}
pip_install west ninja

echo "==> Zephyr $ZEPHYR_VERSION"
cd "$ZEPHYR_WS"
if [ ! -d .west ]; then
    west init --mr "$ZEPHYR_VERSION" .
fi
# shellcheck disable=SC2086
west update --narrow -o=--depth=1 $WEST_MODULES
west zephyr-export
pip_install -r zephyr/scripts/requirements-base.txt

echo "==> Zephyr SDK $SDK_VERSION"
if [ ! -d "$ZEPHYR_SDK_INSTALL_DIR" ]; then
    tarball="zephyr-sdk-${SDK_VERSION}_linux-$(uname -m)_minimal.tar.xz"
    wget -q --show-progress -O "$HOME/$tarball" \
        "https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v$SDK_VERSION/$tarball"
    tar -C "$HOME" -xf "$HOME/$tarball"
    rm -f "$HOME/$tarball"
fi
"$ZEPHYR_SDK_INSTALL_DIR/setup.sh" -t arm-zephyr-eabi -h -c

echo "==> gperf (richiesto da CONFIG_USERSPACE)"
if ! command -v gperf >/dev/null; then
    tmp="$(mktemp -d)"
    trap 'rm -rf "$tmp"' EXIT
    wget -q -O "$tmp/gperf.tar.gz" "https://ftp.gnu.org/pub/gnu/gperf/gperf-$GPERF_VERSION.tar.gz"
    tar -C "$tmp" -xf "$tmp/gperf.tar.gz"
    (cd "$tmp/gperf-$GPERF_VERSION" && ./configure --prefix="$HOME/.local" >/dev/null \
        && make -j"$(nproc)" >/dev/null && make install >/dev/null)
fi

echo "==> Fatto. Ora: make build"
