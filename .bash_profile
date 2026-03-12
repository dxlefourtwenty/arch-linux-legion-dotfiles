#
# ~/.bash_profile
#

[[ -f ~/.bashrc ]] && . ~/.bashrc

if [ -z "$WAYLAND_DISPLAY" ] && [ "$(tty)" = "/dev/tty1" ]; then
  exec sway
fi




# Created by `pipx` on 2026-02-16 11:24:36
export PATH="$PATH:/home/dxle/.local/bin"
