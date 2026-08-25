#!/usr/bin/env bash
set -euo pipefail

random_wallpaper_script="$HOME/.config/hyprlock/scripts/random-wallpaper.sh"
random_wallpaper_link="/tmp/hyprlock-random-wallpaper"
default_wallpaper="$HOME/.config/themes/current/wallpaper.png"
hyprlock_bin="/usr/bin/hyprlock"
ln_bin="/usr/bin/ln"
pgrep_bin="/usr/bin/pgrep"
kill_bin="/usr/bin/kill"
sleep_bin="/usr/bin/sleep"

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

stop_process_tree() {
    local parent_pid="$1"
    local child_pid

    while IFS= read -r child_pid; do
        [ -n "$child_pid" ] || continue
        stop_process_tree "$child_pid"
    done < <("$pgrep_bin" -P "$parent_pid" 2>/dev/null || true)

    "$kill_bin" -TERM "$parent_pid" 2>/dev/null || true
}

stop_hyprlock() {
    local hyprlock_pid

    if ! "$pgrep_bin" -x -u "$EUID" hyprlock >/dev/null 2>&1; then
        return 0
    fi

    while IFS= read -r hyprlock_pid; do
        [ -n "$hyprlock_pid" ] || continue
        stop_process_tree "$hyprlock_pid"
    done < <("$pgrep_bin" -x -u "$EUID" hyprlock)

    for _ in {1..20}; do
        if ! "$pgrep_bin" -x -u "$EUID" hyprlock >/dev/null 2>&1; then
            return 0
        fi
        "$sleep_bin" 0.05
    done

    return 1
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

stop_hyprlock

exec "$hyprlock_bin" "$@"
