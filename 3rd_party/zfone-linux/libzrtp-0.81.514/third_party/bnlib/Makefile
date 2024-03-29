#
# This code is pretty well tested, but not optimizing it will have
# a *major* effect on speed, so its optimzation flags are separate
# from the rest of the release.
#
# For the SPARC v8, at least, gcc produces a *much* faster library than
# the SunPro C compiler.  On a 50 MHz TI TMS390Z50 SuperSPARC:
#  14.5 vs.   47.2 ms per  256-bit modular exponentiation.
#  77.4 vs.  317.8 ms per  512-bit modular exponentiation.
# 249.0 vs. 1031.5 ms per 1024-bit modular exponentiation
#
CC=gcc
# for libzrtp support
CFLAGS=-O0 -g3    -DHAVE_CONFIG_H $(DEFINE) -I../../include -I../../include/enterprise -I../../projects/gnu/config -I../../third_party/bgaes
srcdir=.


# Extra object files (e.g. lbnalpha.o for Alpha assembly routines)
AUXOBJS=
# Extra definitions (e.g. -DBNINCLUDE=lbnalpha.h)
DEFINE=

SHELL = /bin/sh
.SUFFIXES:
.SUFFIXES: .c .h .o .s

LD      =$(CC)
LDFLAGS	=
LIBS=
RANLIB=ranlib

# If you have a machine-specific assembly file, add it to AUXOBJS.
OBJS	= bn00.o lbn00.o bn.o lbnmem.o sieve.o prime.o \
	  bnprint.o jacobi.o germain.o $(AUXOBJS)

BNLIB	= libbn.a 

# for libzrtp support
all: $(BNLIB) bntest32.c bntest64.c
#    bntest germtest

# for libzrtp support
install: all
	test -d /usr/local/include/libzrtp || mkdir /usr/local/include/libzrtp
	cp *.h /usr/local/include/libzrtp
	cp 'libbn.a' '/usr/local/lib/libbn.a'
	chmod 644 /usr/local/lib/libbn.a
	chown 0:0 /usr/local/lib/libbn.a
	$(RANLIB) '/usr/local/lib/libbn.a'

# for libzrtp support
#check: bntest
#	./bntest
check:

bntest: bntest00.o $(BNLIB)
	$(LD) $(LDFLAGS) -o $@ bntest00.o $(BNLIB) $(LIBS)

germtest: germtest.o $(BNLIB)
	$(LD) $(LDFLAGS) -o $@ germtest.o $(BNLIB) $(LIBS)

$(BNLIB): $(OBJS)
	$(AR) r $@ $?
	$(RANLIB) $@

# Here we get tricky... if we're compiling with GCC, then GCCMAGIC1 and
# GCCMAGIC2 are set so that we actually do a link, but with the -r flag
# (produce relocatable output) and with -lgcc *only*.  This is so that
# the result can be linked cleanly with code compiled with another cc,
# which doesn't know about -lgcc.  Presumably the lbnXX.o file, which
# has by far the most math in it, will have a call to every interesting
# support-library function.

lbn00.o: $(srcdir)/lbn00.c $(HDRS) bnconfig.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -I. -I$(srcdir) -o $@ -Wl,-r -nostdlib lbn00.c -lgcc

lbn16.o: $(srcdir)/lbn16.c $(HDRS) bnconfig.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -I. -I$(srcdir) -o $@ -Wl,-r -nostdlib lbn16.c -lgcc

lbn32.o: $(srcdir)/lbn32.c $(HDRS) bnconfig.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -I. -I$(srcdir) -o $@ -Wl,-r -nostdlib lbn32.c -lgcc

lbn64.o: $(srcdir)/lbn64.c $(HDRS) bnconfig.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -I. -I$(srcdir) -o $@ -Wl,-r -nostdlib lbn64.c -lgcc

# The default .o rule.
.c.o: bnconfig.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -I. -I$(srcdir) -o $@ -c $<

# Extra, non-obvious dependencies.  Bnlib can be compiled in three
# word sizes, and the *00.c files #include the right .c files based
# on <limits.h>, which means that a single compilation will only use a
# subset of these files.  Duplicated here in case someone regenerates
# dependencies with cc -M and they get lost.

lbn00.o: lbn16.c lbn32.c lbn64.c lbn16.h lbn32.h lbn64.h 
bn00.o: bn16.c bn32.c bn64.c bn16.h bn32.h bn64.h \
  bninit16.c bninit32.c bninit64.c
bntest00.o: bntest16.c bntest32.c bntest64.c lbn16.h lbn32.h lbn64.h

# Actual build commented out to prevent confusion by people without autoconf.
# Do it manually for now.
configure: configure.in
	@echo "configure script is out of date; run autoconf"
#	autoconf

clean:
	$(RM) -f *.o *32.[ch] *64.[ch]

distclean: clean
	-rm -f Makefile

BNSOURCES = lbn32.c lbn32.h bn32.c bn32.h bninit32.c bntest32.c \
	lbn64.c lbn64.h bn64.c bn64.h bninit64.c bntest64.c

# An explicit target that can be made before distribution for
# machines that don't have sed.
bnsources: $(BNSOURCES)

# The 16-bit versions of the code are the master versions; all else is
# generated from them.  This fiddling about makes them unwriteable
# to discourage improper edits.

# (You didn't know that suffixes for suffix rules didn't have to begin
# with a period, did you?)
.SUFFIXES: 16.c 16.h 32.c 32.h 64.c 64.h
16.c32.c:
	@test ! -f $@ -o -w $@ || chmod u+w $@ && test -w $@ || rm -f $@
	sed -e s/32/64/g -e s/16/32/g $< > $@
	@chmod a-w $@

16.h32.h:
	@test ! -f $@ -o -w $@ || chmod u+w $@ && test -w $@ || rm -f $@
	sed -e s/32/64/g -e s/16/32/g $< > $@
	@chmod a-w $@

16.c64.c:
	@test ! -f $@ -o -w $@ || chmod u+w $@ && test -w $@ || rm -f $@
	sed -e s/32/128/g -e s/16/64/g $< > $@
	@chmod a-w $@

16.h64.h:
	@test ! -f $@ -o -w $@ || chmod u+w $@ && test -w $@ || rm -f $@
	sed -e s/32/128/g -e s/16/64/g $< > $@
	@chmod a-w $@

### Dependencies
bn.o: bn.c bn.h
bn00.o: bn00.c bnsize00.h lbn.h bn16.c bn32.c bn64.c lbn16.c lbn32.h \
  lbn64.h lbnmem.h bn16.h bn32.h bn64.h bn.h kludge.h bninit16.c \
  bninit32.c bninit64.c
bn16.o: bn16.c lbn.h lbn16.h lbnmem.h bn16.h bn.h kludge.h
bn32.o: bn32.c lbn.h lbn32.h lbnmem.h bn32.h bn.h kludge.h
bn64.o: bn64.c lbn.h lbn64.h lbnmem.h bn64.h bn.h kludge.h
bn68000.o: bn68000.c lbn.h lbn68000.h bn16.h bn32.h
bn8086.o: bn8086.c lbn.h bn64.h lbn8086.h bn32.h
bninit16.o: bninit16.c bn.h bn16.h
bninit32.o: bninit32.c bn.h bn32.h
bninit64.o: bninit64.c bn.h bn64.h
bnprint.o: bnprint.c bn.h bnprint.h kludge.h
bntest00.o: bntest00.c bnsize00.h lbn.h bntest16.c bntest32.c \
  bntest64.c cputime.h lbn16.h lbn32.h lbn64.h kludge.h
bntest16.o: bntest16.c cputime.h lbn16.h lbn.h kludge.h
bntest32.o: bntest32.c cputime.h lbn32.h lbn.h kludge.h
bntest64.o: bntest64.c cputime.h lbn64.h lbn.h kludge.h
germain.o: germain.c bn.h germain.h jacobi.h lbnmem.h sieve.h kludge.h
germtest.o: germtest.c bn.h germain.h sieve.h cputime.h bnprint.h
jacobi.o: jacobi.c bn.h jacobi.h
lbn00.o: lbn00.c bnsize00.h lbn.h lbn16.c lbn16.h lbn32.c lbn32.h \
  lbn64.c lbn64.h lbnmem.h kludge.h
lbn16.o: lbn16.c lbn.h lbn16.h lbnmem.h kludge.h
lbn32.o: lbn32.c lbn.h lbn32.h lbnmem.h kludge.h
lbn64.o: lbn64.c lbn.h lbn64.h lbnmem.h kludge.h
lbn68000.o: lbn68000.c lbn.h lbn68000.h
lbn68020.o: lbn68020.c lbn.h lbn68020.h
lbnmem.o: lbnmem.c lbn.h lbnmem.h kludge.h
lbnppc.o: lbnppc.c lbnppc.h ppcasm.h
prime.o: prime.c bn.h lbnmem.h prime.h sieve.h kludge.h
sieve.o: sieve.c bn.h sieve.h kludge.h
sizetest.c: bnsize00.h

distdir:
