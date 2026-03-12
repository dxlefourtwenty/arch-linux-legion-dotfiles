#!/usr/bin/env bash
set -euo pipefail
source ~/.config/swaylock/colors.sh

swaylock \
  --daemonize \
  --screenshots \
  --effect-blur 8x6 \
  --clock \
  --indicator \
  --indicator-radius 110 \
  --indicator-thickness 10 \
  --inside-color "$COLOR_BG" \
  --ring-color "$COLOR_ACCENT" \
  --line-color "$COLOR_ACCENT" \
  --text-color "$COLOR_TEXT" \
  --ring-ver-color "$COLOR_OK" \
  --inside-ver-color "$COLOR_BG" \
  --text-ver-color "$COLOR_OK" \
  --line-ver-color "$COLOR_OK"
