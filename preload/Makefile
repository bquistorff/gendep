# You can also set GENDEP_DEBUG to get debugging output.
# Here are on/off commands
# . export GENDEP_DEBUG=1
# . unset GENDEP_DEBUG
VERSION=0.1-gcc4-fc6-lester
CSRC = deptrack.c syscall.c

CFLAGS=-fPIC -Wall -pedantic -g
DDIR=gendep-$(VERSION)
OBJS= $(CSRC:.c=.o)

libgendep.so: $(OBJS)
	gcc -shared -Wl,-soname,$@ -o $@ $^

clean:
	rm -f *.o libgendep.so *~
