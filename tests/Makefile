# You can also set GENDEP_DEBUG to get debugging output.
# Here are on/off commands
# . export GENDEP_DEBUG=1
# . unset GENDEP_DEBUG

.PHONY: clean

test-stata-preload:
	-rm stata_test_tar.dep
	GENDEP_TARGET='stata_test_tar' \
		GENDEP_FMATCH='-^/usr -^/home/stata -^/home/quistorff/\.' \
		LD_PRELOAD=$(shell pwd)/../preload/libgendep.so\
		stata-mp -b do stata_test.do
	cat stata_test_tar.dep

test-stata-trace:
	GENDEP_TARGET='stata_test_tar' \
		GENDEP_FMATCH='gendep' \
		dep_tracker.sh stata-mp -b do stata_test.do
	cat stata_test_tar.dep
	rm -f stata_test.txt stata.est stata_internal_log.smcl stata_test.log stata_mata.dat Graph.gph auto_loc.dta Graph.eps

test-preload:
	rm -f simple-cat_tar.dep
	GENDEP_TARGET='simple-cat_tar' \
		GENDEP_FMATCH='-^/usr' \
		LD_PRELOAD='../preload/libgendep.so'\
		gcc -c simple-cat.c
	cat simple-cat_tar.dep

test-trace:
	GENDEP_TARGET='simple-cat_tar' \
		GENDEP_FMATCH='gendep' \
		dep_tracker.sh gcc -o simple-cat simple-cat.c
	cat simple-cat_tar.dep

clean:
	rm -f *.dep *.o simple-cat *~


