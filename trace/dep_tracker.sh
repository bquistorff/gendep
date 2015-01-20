#! /bin/bash
#strace v4.5.18 doesn't propagate the error code of the traced process. Always 0
strace -f -s 1 -e trace=read,write,open -o $GENDEP_TARGET.trace $@
if [ $? -eq 0 ]; then 
	rm $GENDEP_TARGET.dep
	strace_log_filter.py $GENDEP_TARGET.trace $GENDEP_TARGET "$GENDEP_FMATCH"
	rm $GENDEP_TARGET.trace
fi
