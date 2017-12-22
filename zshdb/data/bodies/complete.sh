

typeset -a _Dbg_matches; _Dbg_matches=()
typeset -A _Dbg_complete_level_1_data
typeset -A _Dbg_complete_level_2_data

zle -C zshdb_complete menu-expand-or-complete _Dbg_complete_level_0
bindkey '^i' zshdb_complete
