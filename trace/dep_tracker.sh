#! /bin/bash
#strace v4.5.18 doesn't propagate the error code of the traced process. Always 0
strace -f -s 1 -e trace=read,write,open -o $GENDEP_TARGET.trace $@
if [ $? -eq 0 ]; then 
	rm -f $GENDEP_TARGET.dep
	strace_log_filter.py $GENDEP_TARGET.trace $GENDEP_TARGET
	if [ "$GENDEP_DEBUG" != "1" ]; then
		rm $GENDEP_TARGET.trace
	fi
fi
