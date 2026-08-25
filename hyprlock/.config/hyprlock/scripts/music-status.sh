#!/usr/bin/env bash
set -euo pipefail

timeout_seconds="${HYPRLOCK_MUSIC_TIMEOUT_SECONDS:-1}"
max_title_length="${HYPRLOCK_MUSIC_TITLE_MAX_LEN:-24}"
max_artist_length="${HYPRLOCK_MUSIC_ARTIST_MAX_LEN:-30}"

usage() {
    printf 'Usage: music-status.sh --icon|--title|--artist\n'
}

validate_positive_integer() {
    [[ "$1" =~ ^[1-9][0-9]*$ ]]
}

metadata() {
    local format="$1"

    /usr/bin/timeout "${timeout_seconds}s" \
        /usr/bin/playerctl metadata --format "$format" 2>/dev/null \
        || true
}

truncate_value() {
    local value="$1"
    local max_length="$2"

    printf '%s' "$value" | /usr/bin/cut -c1-"$max_length"
}

escape_markup() {
    /usr/bin/sed \
        -e 's/&/\&amp;/g' \
        -e 's/</\&lt;/g' \
        -e 's/>/\&gt;/g'
}

print_icon() {
    local player_name
    player_name="$(metadata '{{playerName}}')"

    case "${player_name,,}" in
        *spotify*)
            printf ' <b> </b> \n'
            ;;
        '')
            printf ' 󰎆 \n'
            ;;
        *)
            printf '  \n'
            ;;
    esac
}

print_title() {
    local title
    title="$(metadata '{{xesam:title}}')"

    if [[ -z "$title" ]]; then
        printf 'No Music Playing\n'
        return
    fi

    truncate_value "$title" "$max_title_length" | escape_markup
    printf '\n'
}

print_artist() {
    local artist
    artist="$(metadata '{{xesam:artist}}')"

    if [[ -z "$artist" ]]; then
        printf '\n'
        return
    fi

    truncate_value "$artist" "$max_artist_length" | escape_markup
    printf '\n'
}

if ! validate_positive_integer "$timeout_seconds" \
    || ! validate_positive_integer "$max_title_length" \
    || ! validate_positive_integer "$max_artist_length"; then
    exit 2
fi

case "${1:-}" in
    --icon)
        print_icon
        ;;
    --title)
        print_title
        ;;
    --artist)
        print_artist
        ;;
    -h|--help)
        usage
        ;;
    *)
        usage >&2
        exit 1
        ;;
esac
