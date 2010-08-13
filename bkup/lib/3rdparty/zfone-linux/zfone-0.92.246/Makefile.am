#
# Copyright (c) 2006 Philip R. Zimmermann. All rights reserved.
# Contact: http://philzimmermann.com
#
# Viktor Krikun <v.krikun@soft-industry.com> <v.krikun@gmail.com>
#


INCLUDES = -I$(top_srcdir)/base -I$(top_srcdir)/src/tcp -I$(top_srcdir)/include\
	   -I$(zrtp_inc) @GTK_CFLAGS@

bin_PROGRAMS = zfone

zfone_SOURCES = src/zfoneg_ctrl.c src/main.c src/zfoneg_cb.c src/zfoneg_listbox.c\
		src/tcp/zfoneg_tcpconn.c src/zfoneg_pref_form.c src/zfoneg_check_form.c\
		src/zfoneg_secure_params.c src/config.c base/zsockets.c base/zcmd_conv.c\
		src/zfoneg_cache_form.c src/zfoneg_utils.c \
		src/zfoneg_compress.c



zfone_LDADD = -lpthread @GTK_LIBS@ -lz

SUBDIRS = resources

EXTRA_DIST = ./base/zcmd_conv.h  ./base/zlist.h ./base/zsockets.h \
	     \
	    include/files.h include/zfoneg_cb.h include/zfoneg_commands.h include/zfoneg_ctrl.h\
	    include/zfoneg_listbox.h include/zfoneg_pref_form.h include/zfoneg_check_form.h\
	    include/zfoneg_config.h include/zfoneg_secure_params.h include/zfoneg_cache_form.h\
	    src/tcp/zfoneg_tcpconn.h include/zfoneg_utils.h ./cfg \
		resources/pictures/sec_green.png resources/pictures/sec_grey.png resources/pictures/sec_red.png \
		resources/pictures/icon_blue.png resources/pictures/icon_green.png resources/pictures/icon_grey.png \
		resources/pictures/icon_red.png resources/pictures/icon_yellow.png resources/pictures/prefs_advanced.png \
		resources/pictures/prefs_crypto.png resources/pictures/prefs_general.png resources/pictures/rx_green.png \
		resources/pictures/rx_grey.png resources/pictures/rx_red.png resources/pictures/state_analyzing.png \
		resources/pictures/state_clear.png resources/pictures/state_error.png \
		resources/pictures/state_hear_ctx.png resources/pictures/state_idle.png resources/pictures/state_looking.png \
		resources/pictures/state_no_filter.png resources/pictures/state_no_net.png resources/pictures/state_no_zrtp.png \
		resources/pictures/state_press_clear.png \
		resources/pictures/state_securing.png  \
		resources/pictures/state_waiting.png resources/pictures/tx_green.png resources/pictures/tx_grey.png \
		resources/pictures/tx_red.png resources/pictures/Zfone.png resources/pictures/icon_blue.png resources/pictures/icon_green.png \
		resources/pictures/icon_grey.png resources/pictures/icon_red.png resources/pictures/icon_yellow.png \
		resources/pictures/prefs_advanced.png resources/pictures/prefs_crypto.png resources/pictures/prefs_general.png \
		resources/pictures/rx_green.png resources/pictures/rx_grey.png resources/pictures/rx_red.png resources/pictures/state_analyzing.png \
		resources/pictures/state_clear.png resources/pictures/state_error.png \
		resources/pictures/state_hear_ctx.png resources/pictures/state_idle.png resources/pictures/state_looking.png \
		resources/pictures/state_no_filter.png resources/pictures/state_no_net.png resources/pictures/state_no_zrtp.png \
		resources/pictures/state_press_clear.png \
		resources/pictures/state_securing.png \
		resources/pictures/state_waiting.png resources/pictures/tx_green.png resources/pictures/tx_grey.png \
		resources/pictures/tx_red.png resources/pictures/Zfone.png \
		resources/pictures/state_secure_mitm.png resources/pictures/state_secure_audio.png resources/pictures/state_secure_video.png \
		resources/pictures/state_secure_oye.png \
		resources/pictures/state_secure_mitm_f.png resources/pictures/state_secure_audio_f.png resources/pictures/state_secure_video_f.png \
		resources/pictures/state_secure_oye_f.png \
		resources/pictures/padlock.png resources/pictures/padlock_disable.png resources/pictures/padlock_checked.png \
		resources/pictures/ripcord.png resources/pictures/state_passive_peer.png \
		./config/zfone.desktop

install-data-hook:
	cp ./config/zfone.desktop /usr/share/applications
	cp ./resources/pictures/Zfone.png /usr/share/icons/hicolor/48x48/apps/

uninstall:
	rm -f $(prefix)/bin/zfone
	rm -rf $(prefix)/share/zfone/pictures
	rm -f /usr/share/icons/hicolor/48x48/apps/Zfone.png 
	rm -f /usr/share/applications/zfone.desktop
