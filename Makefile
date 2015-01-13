VERSION=0.1-gcc4-fc6-lester
CSRC = deptrack.c syscall.c

CFLAGS=-fPIC -Wall -pedantic -g
DDIR=gendep-$(VERSION)
OBJS= $(CSRC:.c=.o)

libgendep.so: $(OBJS)
	gcc -shared -Wl,-soname,$@ -o $@ $^

test-stata:
	rm simple-stata.dep
	GENDEP_TARGET='simple-stata' \
		GENDEP_FMATCH='-^/usr' \
		LD_PRELOAD=$(shell pwd)/libgendep.so\
		stata-mp -b do stata_test.do
	rm stata_test.txt stata.est stata_internal_log.smcl stata_test.log stata_mata.dat Graph.gph auto_loc.dta Graph.eps
	cat simple-stata.dep
	@echo

test:
	rm simple-cat.dep badexp.dep
	GENDEP_TARGET='simple-cat' \
		GENDEP_FMATCH='+\.h$$ -^/usr' \
		LD_PRELOAD=$(shell pwd)/libgendep.so\
		gcc -o simple-cat simple-cat.c
	GENDEP_TARGET='badexp' \
		GENDEP_FMATCH='\)foo'\
		LD_PRELOAD=$(shell pwd)/libgendep.so\
		gcc -o simple-cat simple-cat.c
	@echo ====================
	@echo == simple-cat.dep ==
	cat simple-cat.dep
	@echo
	@echo ================
	@echo == badexp.dep ==
	cat badexp.dep
	@echo
	@echo ================

clean:
	rm -f *.dep *.o simple-cat libgendep.so *~


dist:
	rm -rf $(DDIR)
	mkdir $(DDIR)
	ln $(CSRC) gendep.h Makefile simple-cat.c README $(DDIR)
	tar cfz $(DDIR).tar.gz $(DDIR)
	rm -rf $(DDIR)
