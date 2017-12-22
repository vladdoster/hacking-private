

zmodload -ap zsh/mapfile mapfile &> /dev/null

typeset _Dbg_bogus_file=' A really bogus file'
typeset -A _Dbg_filenames
typeset -A _Dbg_file2canonic
typeset -A _Dbg_fileinfo
_Dbg_filecache_reset
