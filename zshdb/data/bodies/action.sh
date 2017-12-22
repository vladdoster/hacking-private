
typeset -i _Dbg_action_count=0
typeset -a  _Dbg_action_enable; _Dbg_action_enable=()
typeset -a  _Dbg_action_file; _Dbg_action_file=()
typeset -a _Dbg_action_line; _Dbg_action_line=()
typeset -a  _Dbg_action_stmt; _Dbg_action_stmt=()
typeset -i  _Dbg_action_max=0
typeset -A _Dbg_action_file2action; _Dbg_action_file2action=()
typeset -A _Dbg_action_file2linenos; _Dbg_action_file2linenos=()
