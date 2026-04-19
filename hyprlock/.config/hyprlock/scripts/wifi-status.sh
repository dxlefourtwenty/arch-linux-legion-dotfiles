#!/usr/bin/env bash
set -euo pipefail

CACHE_FILE="/tmp/hyprlock-wifi-status.cache"
TS_FILE="/tmp/hyprlock-wifi-status.ts"
LOCK_FILE="/tmp/hyprlock-wifi-status.lock"

CACHE_TTL_SECONDS="${HYPRLOCK_WIFI_CACHE_TTL:-8}"
FETCH_TIMEOUT_SECONDS="${HYPRLOCK_WIFI_FETCH_TIMEOUT:-2}"
MAX_LENGTH="${HYPRLOCK_WIFI_MAX_LEN:-14}"

now_epoch() {
    /usr/bin/date +%s
}

read_cache() {
    if [[ -f "$CACHE_FILE" ]]; then
        /usr/bin/cat "$CACHE_FILE"
    fi
}

write_cache() {
    local value="$1"
    printf '%s\n' "$value" > "$CACHE_FILE"
    printf '%s\n' "$(now_epoch)" > "$TS_FILE"
}

is_cache_fresh() {
    [[ -f "$TS_FILE" ]] || return 1

    local cached_at now age
    cached_at=$(/usr/bin/cat "$TS_FILE" 2>/dev/null || printf '0')
    now="$(now_epoch)"
    age=$((now - cached_at))
    (( age <= CACHE_TTL_SECONDS ))
}

truncate_value() {
    local value="$1"
    printf '%s' "$value" | /usr/bin/cut -c1-"$MAX_LENGTH"
}

wifi_device() {
    /usr/bin/timeout "${FETCH_TIMEOUT_SECONDS}s" \
        /usr/bin/nmcli -t -f DEVICE,TYPE device status 2>/dev/null \
        | /usr/bin/awk -F: '$2=="wifi"{print $1; exit}'
}

wifi_from_active_connections() {
    local wifi
    wifi=$(
        /usr/bin/timeout "${FETCH_TIMEOUT_SECONDS}s" \
        /usr/bin/nmcli -t -f NAME,TYPE connection show --active 2>/dev/null \
        | /usr/bin/awk -F: '$2=="802-11-wireless"{print $1; exit}' \
        || true
    )

    if [[ -n "$wifi" ]]; then
        truncate_value "$wifi"
        return 0
    fi

    return 1
}

fetch_wifi_value() {
    local device info state connection
    device="$(wifi_device || true)"
    [[ -n "$device" ]] || return 1

    info=$(
        /usr/bin/timeout "${FETCH_TIMEOUT_SECONDS}s" \
        /usr/bin/nmcli -g GENERAL.STATE,GENERAL.CONNECTION device show "$device" 2>/dev/null \
        || true
    )

    [[ -n "$info" ]] || return 1

    state="$(printf '%s\n' "$info" | /usr/bin/sed -n '1p')"
    connection="$(printf '%s\n' "$info" | /usr/bin/sed -n '2p')"

    if [[ "$state" == *"(connected)"* && -n "$connection" && "$connection" != "--" ]]; then
        truncate_value "$connection"
        return 0
    fi

    if [[ "$state" == *"(disconnected)"* || "$state" == *"(unavailable)"* || "$state" == *"(unmanaged)"* ]]; then
        printf 'Disconnected'
        return 0
    fi

    wifi_from_active_connections
}

get_display_value() {
    local cached
    cached="$(read_cache || true)"
    if [[ -n "$cached" ]]; then
        if [[ "$cached" == "Disconnected" ]] && ! is_cache_fresh; then
            printf 'Checking...\n'
            return
        fi
        printf '%s\n' "$cached"
        return
    fi

    printf 'Checking...\n'
}

refresh_cache_async() {
    (
        exec 9>"$LOCK_FILE"
        if ! /usr/bin/flock -n 9; then
            exit 0
        fi

        if is_cache_fresh; then
            exit 0
        fi

        local latest
        latest="$(fetch_wifi_value || true)"
        [[ -n "$latest" ]] || exit 0
        write_cache "$latest"
    ) >/dev/null 2>&1 &
}

get_display_value
if ! is_cache_fresh; then
    refresh_cache_async
fi
