#! /bin/bash
#strace v4.5.18 doesn't propagate the error code of the traced process. Always 0
strace -f -s 1 -e trace=read,write,open,dup,dup2,dup3 -o $DEP_FILE.trace $@
if [ $? -eq 0 ]; then 
	rm -f $DEP_FILE
	strace_log_filter.py $DEP_FILE.trace $DEP_FILE
	if [ "$GENDEP_DEBUG" != "1" ]; then
		rm $DEP_FILE.trace
	fi
fi
