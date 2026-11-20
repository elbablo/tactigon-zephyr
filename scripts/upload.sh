#!/usr/bin/env bash
# Copia il firmware UF2 sul bootloader della XIAO nRF52840 Sense.
# La scheda deve essere in modalita' bootloader (doppio click su reset):
# compare il drive XIAO-SENSE. Mount alternativo: UF2_MOUNT=/percorso make upload
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
UF2="${1:-$SCRIPT_DIR/../build/zephyr/zephyr.uf2}"

[ -f "$UF2" ] || { echo "Firmware non trovato: $UF2 (esegui prima 'make build')" >&2; exit 1; }

mount_point="${UF2_MOUNT:-}"
if [ -z "$mount_point" ]; then
    for dir in "/media/$USER/XIAO-SENSE" "/run/media/$USER/XIAO-SENSE" "/mnt/XIAO-SENSE"; do
        if [ -d "$dir" ]; then mount_point="$dir"; break; fi
    done
fi

if [ -z "$mount_point" ] || [ ! -d "$mount_point" ]; then
    echo "Drive XIAO-SENSE non trovato. Premi due volte reset sulla scheda e riprova." >&2
    exit 1
fi

echo "Copio $(basename "$UF2") su $mount_point"
# La scheda si riavvia e smonta il drive a fine copia: eventuali errori finali sono attesi
cp "$UF2" "$mount_point/" || true
sync 2>/dev/null || true
echo "Upload completato."
