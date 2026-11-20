# Ambiente di build Zephyr. Da includere con: . scripts/env.sh
# Ogni variabile puo' essere sovrascritta dall'esterno.

: "${ZEPHYR_VERSION:=v4.3.2}"
: "${SDK_VERSION:=0.17.4}"
: "${ZEPHYR_WS:=$HOME/zephyrproject}"
: "${ZEPHYR_SDK_INSTALL_DIR:=$HOME/zephyr-sdk-$SDK_VERSION}"
: "${BOARD:=xiao_ble/nrf52840/sense}"

export ZEPHYR_VERSION SDK_VERSION ZEPHYR_WS ZEPHYR_SDK_INSTALL_DIR BOARD
export ZEPHYR_BASE="$ZEPHYR_WS/zephyr"
export PATH="$HOME/.local/bin:$PATH"

if [ -f "$ZEPHYR_WS/.venv/bin/activate" ]; then
    . "$ZEPHYR_WS/.venv/bin/activate"
fi
