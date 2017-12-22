
typeset _Dbg_old_setopts
typeset _Dbg_old_EXIT_handler=''  

typeset -i _Dbg_QUIT_ON_QUIT=0
typeset -i _Dbg_program_exit_code=0
typeset -a _Dbg_sig_print; _Dbg_sig_print=()
typeset -a _Dbg_sig_stop; _Dbg_sig_stop=()
typeset -a _Dbg_sig_show_stack; _Dbg_sig_show_stack=()
typeset -a _Dbg_sig_passthrough; _Dbg_sig_passthrough=()
typeset -i _Dbg_return_level=0
typeset -a _Dbg_arg; _Dbg_arg=()
