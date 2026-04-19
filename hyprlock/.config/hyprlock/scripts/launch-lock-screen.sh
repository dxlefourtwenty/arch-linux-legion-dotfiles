#!/usr/bin/env bash
set -euo pipefail

random_wallpaper_script="$HOME/.config/hyprlock/scripts/random-wallpaper.sh"
random_wallpaper_link="/tmp/hyprlock-random-wallpaper"
default_wallpaper="$HOME/.config/themes/current/wallpaper.png"
hyprlock_bin="/usr/bin/hyprlock"
ln_bin="/usr/bin/ln"
pidof_bin="/usr/bin/pidof"

dry_run=0

usage() {
    echo "Usage: launch-lock-screen.sh [--dry-run] [--] [hyprlock args...]"
}

refresh_random_wallpaper() {
    if "$random_wallpaper_script" --link "$random_wallpaper_link" >/dev/null 2>&1; then
        return 0
    fi

    [ -e "$default_wallpaper" ] || return 1
    "$ln_bin" -sfn -- "$default_wallpaper" "$random_wallpaper_link"
}

while [ "$#" -gt 0 ]; do
    case "$1" in
        --dry-run)
            dry_run=1
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        --)
            shift
            break
            ;;
        *)
            break
            ;;
    esac
done

if [ "$dry_run" -eq 1 ]; then
    refresh_random_wallpaper || true
    echo "$hyprlock_bin $*"
    exit 0
fi

refresh_random_wallpaper || true

if "$pidof_bin" hyprlock >/dev/null 2>&1; then
    exit 0
fi

"$hyprlock_bin" "$@"
