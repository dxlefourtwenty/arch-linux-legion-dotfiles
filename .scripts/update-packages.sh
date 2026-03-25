#!/usr/bin/env bash
set -euo pipefail

PKG_DIR="$HOME/dotfiles/.packages"

echo "==> Updating system..."
sudo pacman -Syu

echo "==> Updating AUR..."
if command -v yay &> /dev/null; then
  yay -Syu
fi

echo "==> Exporting package lists..."

mkdir -p "$PKG_DIR"

pacman -Qqe | sort > "$PKG_DIR/pkglist.txt"
pacman -Qqm | sort > "$PKG_DIR/aurlist.txt"

echo "==> Done. Package lists updated."
