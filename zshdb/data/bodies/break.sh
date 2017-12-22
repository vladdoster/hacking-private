

typeset -a _Dbg_keep
_Dbg_keep=('keep' 'del')
typeset -a _Dbg_brkpt_line; _Dbg_brkpt_line=()
typeset -i _Dbg_brkpt_count=0
typeset -a  _Dbg_brkpt_file; _Dbg_brkpt_file=()
typeset -a  _Dbg_brkpt_enable; _Dbg_brkpt_enable=()
typeset -a _Dbg_brkpt_counts; _Dbg_brkpt_counts=()
typeset -a _Dbg_brkpt_onetime; _Dbg_brkpt_onetime=()
typeset -a  _Dbg_brkpt_cond; _Dbg_brkpt_cond=()
typeset -i  _Dbg_brkpt_max=0
typeset -A _Dbg_brkpt_file2linenos; _Dbg_brkpt_file2linenos=()
typeset -A _Dbg_brkpt_file2brkpt; _Dbg_brkpt_file2brkpt=()
