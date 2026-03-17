#
# ~/.bash_profile
#

if [[ -z $DISPLAY ]] && [[ "$(tty)" == "/dev/tty1" ]]; then
  exec start-hyprland
fi

[[ -f ~/.bashrc ]] && . ~/.bashrc

# Created by `pipx` on 2026-02-16 11:24:36
export PATH="$PATH:/home/dxle/.local/bin"

export PATH=$PATH:/home/dxle/.spicetify
