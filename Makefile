# You can also set GENDEP_DEBUG to get debugging output.
# Here are on/off commands
# . export GENDEP_DEBUG=1
# . unset GENDEP_DEBUG

export PATH := $(shell pwd)/trace:$(PATH)

.PHONY: libgendep test-stata-preload test-stata-trace test-preload test-trace

libgendep:
	cd preload && $(MAKE)

test-stata-preload:
	cd tests && $(MAKE) test-stata-preload

test-stata-trace:
	cd tests && $(MAKE) test-stata-trace

test-preload:
	cd tests && $(MAKE) test-preload

test-trace:
	cd tests && $(MAKE) test-trace

clean:
	cd preload && $(MAKE) clean
	cd tests && $(MAKE) clean

dist:
	rm -rf $(DDIR)
	mkdir $(DDIR)
	ln $(CSRC) gendep.h Makefile simple-cat.c README $(DDIR)
	tar cfz $(DDIR).tar.gz $(DDIR)
	rm -rf $(DDIR)
