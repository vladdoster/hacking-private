

typeset -a _Dbg_yn; _Dbg_yn=("n" "y")
typeset -a _Dbg_eval_re;
_Dbg_eval_re=(
    '^[ \t]*(if|elif)[ \t]+([^;]*)((;[ \t]*then?)?|$)'
    '^[ \t]*return[ \t]+(.*)$'
    '^[ \t]*while[ \t]+([^;]*)((;[ \t]*do?)?|$)'
    '^[ \t]*[A-Za-z_][A-Za-z[_0-9]*[-+\]]?=(.*$)'
)
