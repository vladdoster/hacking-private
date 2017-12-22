
[[ -z $_Dbg_debugger_set_opts ]] && \
  typeset -r _Dbg_debugger_set_opts='extendedhistory extendedglob shwordsplit ksharrays histignoredups zle rematchpcre bashrematch'
[[ -z $_Dbg_debugger_unset_opts ]] && \
  typeset -r _Dbg_debugger_unset_opts='localtraps'
[[ -z $_Dbg_check_opts ]] && \
  typeset -r _Dbg_check_opts="$_Dbg_debugger_set_opts $_Dbg_debugger_unset_opts"
