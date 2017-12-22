

if [[ 0 == ${#funcfiletrace[@]} ]] ; then
    unset list
    print_columns
    typeset -a list
    print_columns
    list=(oneitem); print_columns
    list=(a 2 c) print_columns 10 ', '
    list=(
 1   two three
 for 5   six
 7   8) print_columns 12 

    list=(
 one two three
 4ne 5wo 6hree
 7ne 8wo 9hree
 10e 11o 12ree) print_columns 18 

    list=(
 1   two 3
 for 5   six
 7   8) print_columns 12

    list=(
	argv
	cdpath
	fignore
	fpath      
	funcfiletrace
	funcstack
	lib_opts    
	libdir     
	list       
	mailpath
	manpath     
	module_path
	o_annotate
	o_basename
	o_cmdfile   
	o_help    
	o_nx       
	o_quiet
	o_version
	path    
	pipestatus
	psvar   
	signals
	split_result
	temp 
	watch)
  print_columns 80 ' | '

fi
