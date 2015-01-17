#! /bin/bash
strace -f -s 1 -e trace=read,write,open -o $GENDEP_TARGET.trace $@
rm $GENDEP_TARGET.dep
strace_log_filter.py $GENDEP_TARGET.trace $GENDEP_TARGET "$GENDEP_FMATCH"
rm $GENDEP_TARGET.trace

