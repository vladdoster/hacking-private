

if [[ $0 == *sorting.sh ]] ; then 
    [[ -n $ZSH_VERSION ]] && setopt ksharrays
    typeset -a list
    list=()
    sort_list -1 0 
    typeset -p list
    list=('one')
    typeset -p list
    sort_list 0 0 
    typeset -p list
    list=('one' 'two' 'three')
    sort_list 0 2
    typeset -p list
    list=(4 3 2 1)
    sort_list 0 3
    typeset -p list
fi
