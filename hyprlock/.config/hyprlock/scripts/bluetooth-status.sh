#!/usr/bin/env bash
set -euo pipefail

timeout_seconds="${HYPRLOCK_BLUETOOTH_TIMEOUT_SECONDS:-1}"
max_length="${HYPRLOCK_BLUETOOTH_MAX_LEN:-14}"

validate_positive_integer() {
    [[ "$1" =~ ^[1-9][0-9]*$ ]]
}

has_adapter() {
    compgen -G '/sys/class/bluetooth/hci*' >/dev/null
}

connected_device_name() {
    local devices

    devices=$(
        /usr/bin/timeout "${timeout_seconds}s" \
            /usr/bin/bluetoothctl devices Connected 2>/dev/null \
            || true
    )

    printf '%s\n' "$devices" \
        | /usr/bin/awk '$1 == "Device" {$1 = $2 = ""; sub(/^[[:space:]]+/, ""); print; exit}'
}

truncate_value() {
    printf '%s' "$1" | /usr/bin/cut -c1-"$max_length"
}

if ! validate_positive_integer "$timeout_seconds" \
    || ! validate_positive_integer "$max_length"; then
    exit 2
fi

if has_adapter; then
    device_name="$(connected_device_name)"
else
    device_name=""
fi

if [[ -n "$device_name" ]]; then
    truncate_value "$device_name"
    printf '\n'
else
    printf 'Disconnected\n'
fi
