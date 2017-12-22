

if [[ 0 == ${#funcfiletrace[@]} ]] ; then
    dirname=${0%/*}
    [[ $dirname == $0 ]] && _Dbg_libdir='..' || _Dbg_libdir=${dirname}/..
    source $_Dbg_libdir/dbg-opts
fi

typeset -i _Dbg_hi_last_stop=-1
typeset -i _Dbg_set_history=1
