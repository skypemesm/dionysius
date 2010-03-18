#
# Copyright (c) 2006-2008 Philip R. Zimmermann. All rights reserved.
# Contact: http://philzimmermann.com
# 
# Viktor Krikun <v.krikun@soft-industry.com> <v.krikun@gmail.com>
#

INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/include/daemon -I$(top_srcdir)/include/utils\
	   -I$(top_srcdir)/include/zrtp_iface -I$(top_builddir)/config -I$(zrtp_inc)\
	   -I$(top_srcdir)/include/daemon/tcp 
	   
#	   -I$(top_srcdir)/include/testing

# adding libipq for packets capturing on linux
if IS_LINUX_BASED
    INCLUDES += -I$(ipq_inc)
endif

bin_PROGRAMS = zfoned

zfoned_SOURCES = main.c \
		 daemon/zfoned.c daemon/zfoned_cfg.c\
		 daemon/zfoned_engine.c  daemon/zfoned_commands.c\
		 daemon/zfoned_cmdconv.c daemon/zfoned_updater.c\
		 \
		 engine/zfone_heurist.c engine/zfone_manager.c engine/zfone_detector.c\
		 engine/zfone_siparser.c engine/zfone_sipmachine.c\
		 engine/zfone_commands.c engine/zfone_checker.c engine/zfone_heurist_sip.c\
		 engine/zfone_handler.c engine/zfone_heurist_utils.c \
		 \
		 utils/zfone_utils.c\
		 utils/zrtp_poll.c\
		 \
		 zrtp_iface/unix/zfone_alert.c\
		 zrtp_iface/unix/zfone_log.c\
		 zrtp_iface/unix/zfone_net.c \
		 zrtp_iface/unix/zfone_cache.c
		 
if IS_LINUX_BASED
    zfoned_SOURCES += daemon/zfoned_sockets.c\
		daemon/zfoned_net_lin.c daemon/zfoned_system_linux.c daemon/zfoned_ctrl_lin.c
else
    zfoned_SOURCES += daemon/zfoned_net_mac.c daemon/zfoned_system_mac.c daemon/zfoned_ctrl_mac.c
endif

zfoned_LDADD = $(zrtp_lib)/libzrtp.a  $(bn_lib)/libbn.a -lpthread
if IS_LINUX_BASED
    zfoned_LDADD += -lipq -lrt
endif


if COMPILE_MACGREW     
zfoned_LDADD += $(srtp_lib)/libsrtp.a
endif

SUBDIRS = .
