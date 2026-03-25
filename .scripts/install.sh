#!/usr/bin/env bash
set -euo pipefail

PKG_DIR="$HOME/dotfiles/.packages"

echo "==> Installing official packages..."

if [[ ! -f "$PKG_DIR/pkglist.txt" ]]; then
  echo "pkglist.txt not found!"
  exit 1
fi

sudo pacman -Syu --needed --noconfirm - < "$PKG_DIR/pkglist.txt"

echo "==> Ensuring yay is installed..."

if ! command -v yay &> /dev/null; then
  sudo pacman -S --needed --noconfirm git base-devel

  tmpdir=$(mktemp -d)
  git clone https://aur.archlinux.org/yay.git "$tmpdir/yay"
  cd "$tmpdir/yay"
  makepkg -si --noconfirm
  cd -
fi

echo "==> Installing AUR packages..."

if [[ -f "$PKG_DIR/aurlist.txt" ]]; then
  yay -S --needed --noconfirm - < "$PKG_DIR/aurlist.txt"
else
  echo "No aurlist.txt found, skipping AUR..."
fi

echo "==> Done."
