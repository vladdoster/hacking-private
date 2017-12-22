

typeset _Dbg_set_str='set --'
typeset -i _Dbg__i
for (( _Dbg__i=0 ; _Dbg__i<${#_Dbg_arg[@]}; _Dbg__i++ )) ; do
  local dq_argi="${_Dbg_arg[_Dbg__i]}"
  _Dbg_set_str="$_Dbg_set_str \"$dq_argi\""
done
eval $_Dbg_set_str

_Dbg_restore_user_vars
_Dbg_set_dol_q $_Dbg_debugged_exit_code
