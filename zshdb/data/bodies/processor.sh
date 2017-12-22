
typeset -i  _Dbg_inside_skip=0
typeset -A _Dbg_cmdloop_hooks
_Dbg_cmdloop_hooks['display']='_Dbg_eval_all_display'

typeset _Dbg_prompt_str='$_Dbg_debugger_name${_Dbg_less}$_Dbg_cmd_num${_Dbg_greater}'
typeset _Dbg_last_cmd=''
typeset _Dbg_cmd=''
typeset -i _Dbg_cmd_num=0

typeset last_next_step_cmd='s'

typeset _Dbg_last_print=''
typeset _Dbg_last_printe=''
typeset -i _Dbg_in_vared=0
typeset -xi _Dbg_fdi ;
typeset -a _Dbg_fd ; _Dbg_fd=()
typeset -a _Dbg_cmdfile ; _Dbg_cmdfile=('')
