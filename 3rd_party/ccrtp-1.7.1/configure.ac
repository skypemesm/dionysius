# Copyright (C) 1999-2005 Open Source Telecom Corporation.
# Copyright (C) 2006-2008 David Sugar, Tycho Softworks.
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

AC_INIT(src/ccrtp/rtp.h)

cfg_options=""
if test -z "$*" ; then
        if test -f ~/.configure ; then
		cfg_options=`grep ^ccrtp: ~/.configure | sed -e s/^ccrtp://`
        elif test -f /etc/configure.conf ; then
		cfg_options=`grep ^ccrtp: /etc/configure.conf | sed -e s/^ccrtp://`
        fi
fi

if test ! -z "$cfg_options" ; then
        echo "using ~/.configure defaults...$cfg_options"
        case "$SHELL" in
        */bash*)
                exec $SHELL $0 $cfg_options
                exit -1
                ;;
        esac
        exec $0 $cfg_options
        exit -1
fi

VERSION="1.7.1"
REQUIRES="1.3.0"
LT_RELEASE="1.7"
LT_VERSION="0:1"
AC_CONFIG_AUX_DIR(autoconf)
AC_CANONICAL_SYSTEM
AC_PROG_CPP
AC_PROG_CXXCPP
AC_PROG_CXX
# OST_PROG_CC_POSIX
AM_PROG_LIBTOOL
AM_INIT_AUTOMAKE(ccrtp,[$VERSION])
AM_CONFIG_HEADER(src/config.h)

AC_C_RESTRICT
AC_C_VOLATILE
AC_C_INLINE

if test -z "$PKG_CONFIG_PATH" ; then
        PKG_CONFIG_PATH="/usr/local/lib/pkgconfig"
fi

if test "$prefix" != "NONE" ; then
        PKG_CONFIG_PATH="$prefix/lib/pkgconfig:$PKG_CONFIG_PATH"
fi

export PKG_CONFIG_PATH

# use "-no-undefined" on Cygwin to force (trigger) libtool to create
# the shared lib. If this is not set this library
# is not created. Be sure that the LIBS variable above contains _all_
# libraries necessary to build ours, Cygwin does not allow undefined
# symbols.
case $host in
#  *-*-msdos* | *-*-go32* | *-*-mingw32* | *-*-cygwin* | *-*-windows*)
  *-*-cygwin*)
	LDFLAGS="$LDFLAGS -no-undefined"
    ;;
  *)
    ;;
esac

# SRTP support
AC_ARG_ENABLE(srtp,
    AS_HELP_STRING([--enable-srtp],
        [enable SRTP support (default enabled)]))
if test -z "$enable_srtp"; then
   enable_srtp="yes"
   AC_DEFINE(SRTP_SUPPORT, [], [SRTP support])
fi

# availability of gcrypt or openSSL crypto library if SRTP support is enabled
if test "${enable_srtp}" = "yes"
then
    openssl="false"
    gcrypt="false"
    AM_PATH_LIBGCRYPT_CCRTP([1:1.2.3],
                  [AC_CHECK_HEADERS([gcrypt.h],
                                    [], AC_MSG_ERROR([libgcrypt headers not found.]))
                   LIBS="$LIBGCRYPT_LIBS $LIBS"
                   gcrypt="true"
                  ],
                  [AC_CHECK_LIB([crypto],
                      [EVP_CipherInit_ex],
                      [AC_CHECK_HEADERS([openssl/bn.h openssl/aes.h openssl/sha.h],
                                        [], AC_MSG_ERROR([OpenSSL headers not found.]))
                       LIBS="-lcrypto $LIBS"
                       openssl="true"
                      ],
                      [AC_MSG_ERROR([Cannot find libgcrypt or OpenSSL crypto library.])]
                     )
                  ]
                 )
    AM_CONDITIONAL(SRTP_OPENSSL, test "$openssl" = "true")
    AM_CONDITIONAL(SRTP_GCRYPT, test "$gcrypt" = "true")

fi

OST_PROG_COMMON
OST_PROG_LIBVER
OST_PROG_LIBRARY(CCXX,[$LT_VERSION])
OST_AUTOMAKE_MODE
OST_MAINTAINER_MODE
PKG_CHECK_MODULES(COMMON, libccgnu2 >= $REQUIRES)
CXXFLAGS="$CXXFLAGS $COMMON_CFLAGS"
COMMON_LIBPATH=`pkg-config --variable=libdir libccgnu2`
GNULIBS="-L$COMMON_LIBPATH $COMMON_LIBS $LIBS"
AC_SUBST(GNULIBS)
# OST_CC_ENDIAN - now in Common C++
# Are we using the GNU compiler?
if test $GCC = yes ; then
	WARN_FLAGS="-Wall -ansi -pedantic"
else
	WARN_FLAGS=""
fi
AC_SUBST(WARN_FLAGS)

OST_DEBUG

dnl --with-nana: check for nana, the GNU assertion checking and logging library
AC_ARG_WITH(nana,[  --with-nana             use GNU nana for asserts and logging],[
	AC_CHECK_HEADER(nana.h, [
		AC_CHECK_HEADERS(DL.h, [
			AC_CHECK_LIB(nana, main)
			])
	])
])

KDOC_DIR="\$(top_srcdir)/doc"
AC_SUBST(LT_RELEASE)
AC_SUBST(KDOC_DIR)

AC_PATH_PROG(DOXYGEN, doxygen, no)
AC_SUBST(DOXYGEN)
AM_CONDITIONAL(DOXY, test "$DOXYGEN" != "no")

LIB_VERSION=`echo $LT_RELEASE | sed -e 's/[\.]/_/'` 
LIB_MAJOR=`echo $LT_VERSION | sed -e 's/[:].*$//'`

AC_SUBST(LIB_VERSION)
AC_SUBST(LIB_MAJOR)

AC_OUTPUT(Makefile src/Makefile src/ccrtp/Makefile m4/Makefile
doc/Makefile demo/Makefile phone/Makefile w32/Makefile ccrtp.spec
ccrtp.list libccrtp1.pc src/ccrtp/crypto/Makefile)
