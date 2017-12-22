

typeset -a _Dbg_dir
_Dbg_dir=('\$cdir' '\$cwd' )
[[ -z ${_Dbg_cdir} ]] && typeset _Dbg_cdir=${_Dbg_source_file%/*}
[[ -z ${_Dbg_cdir} ]] && typeset _Dbg_cdir=$(pwd)
