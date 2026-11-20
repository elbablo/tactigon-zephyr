#!/usr/bin/env bash
# Apre la console seriale (CDC ACM) della scheda.
# Variabili: SERIAL_PORT=/dev/ttyACM1  BAUD=115200
# Uscita: minicom = Ctrl-A poi X; screen = Ctrl-A poi K.
set -euo pipefail

BAUD="${BAUD:-115200}"
port="${SERIAL_PORT:-}"

find_port() {
    local p
    for p in /dev/serial/by-id/*Zephyr* /dev/ttyACM*; do
        [ -e "$p" ] && { echo "$p"; return 0; }
    done
    return 1
}

if [ -z "$port" ]; then
    # Dopo un upload la scheda si riavvia e la porta riappare dopo qualche secondo
    for _ in $(seq 1 20); do
        port="$(find_port || true)"
        [ -n "$port" ] && break
        sleep 0.5
    done
fi

[ -n "$port" ] || { echo "Nessuna porta seriale trovata (scheda collegata? In modalita' bootloader non c'e' console)." >&2; exit 1; }

if [ ! -r "$port" ] || [ ! -w "$port" ]; then
    echo "Nessun permesso su $port. Aggiungiti al gruppo dialout e rifai il login:" >&2
    echo "  sudo usermod -aG dialout $USER" >&2
    exit 1
fi

echo "Connessione a $port @ $BAUD"
if command -v minicom >/dev/null; then
    exec minicom -D "$port" -b "$BAUD"
elif command -v screen >/dev/null; then
    exec screen "$port" "$BAUD"
else
    echo "Installa minicom o screen." >&2
    exit 1
fi
