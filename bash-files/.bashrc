#
# ~/.bashrc
#

# If not running interactively, don't do anything
[[ $- != *i* ]] && return

if [ -f ~/.bash_aliases ]; then
  . ~/.bash_aliases
fi

if [ -f ~/.bash_env ]; then
  . ~/.bash_env
fi

if [ -f ~/.bash_functions ]; then
  . ~/.bash_functions
fi

# print_newline_if_needed() {
#   if [ -n "$LAST_PROMPT" ]; then
#     echo
#   fi
#   LAST_PROMPT=1
# }
#
# __force_prompt() {
#   return 0
# }

# PROMPT_COMMAND="print_newline_if_needed"

# Enable bash completion
[[ -f /usr/share/bash-completion/bash_completion ]] && source /usr/share/bash-completion/bash_completion
bind 'set show-all-if-ambiguous on'
bind 'set menu-complete-display-prefix on'
bind '"\t": menu-complete'

eval "$(starship init bash)"

