### custom aliases

# bash and nvim
alias vim='nv'
alias edit-bash='nv ~/.bashrc'
alias rel-bash='source ~/.bashrc'
alias bashrc='edit-bash'
alias vsbashrc='code ~/.bashrc'
alias editbash='edit-bash'
alias bashrel='rel-bash'
alias relbash='rel-bash'
alias bash-aliases='vim ~/.bash_aliases'
alias edit-bash-aliases="bash-aliases"
alias bashaliases='bash-aliases'
alias nv='nvim'
alias neovim='nvim'
alias bashenv='nvim ~/.bash_env'

# git
alias git-log='git log --oneline --abbrev-commit --all --graph --decorate --color'

# fd-find
alias fd='fdfind'
export FZF_DEFAULT_COMMAND='fdfind -H'
export FZF_CTRL_T_COMMAND='fdfind -H'
export FZF_ALT_C_COMMAND='fdfind -H -t d'

# rm/trash
alias rm='trash'

# ls
alias ll='eza --icons --color=always -l'
alias la='eza --icons --color=always -a'
alias l='eza --icons --color=always'
alias ls='eza --icons --color=always -l'
alias lla='eza --icons --color=always -la'

# grep
alias grep='grep --color=auto'
alias fgrep='fgrep --color=auto'
alias egrep='egrep --color=auto'

# venv
alias in-venv='[ -n "$VIRTUAL_ENV" ] && echo "venv ON" || echo "venv OFF"'

# themes
alias themesdir='cd ~/.config/themes'

# fastfetch
alias ff2='fastfetch -l arch_small --config nyarch'
alias ff='fastfetch -l arch2'

# directories
alias home='cd /home/dxle'

# fdfind
alias fd-nvim='fd --type f --hidden --exclude .git | fzf | xargs -r nvim'

# open image
alias view-image='~/bin/launch-image-viewer'
alias open-image='~/bin/launch-image-viewer'
alias imv='~/bin/launch-image-viewer'

# open file explorer
alias start='~/bin/launch-file-explorer'
alias open='~/bin/launch-file-explorer'
alias fe='~/bin/launch-file-explorer'

