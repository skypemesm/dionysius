#
# Copyright (c) 2006-2008 Philip R. Zimmermann. All rights reserved.
# Contact: http://philzimmermann.com
# 
# Viktor Krikun <v.krikun@soft-industry.com> <v.krikun@gmail.com>
#

if IS_LINUX_BASED
SUBDIRS = src resources

EXTRA_DIST = include/daemon/sfiles.h include/daemon/zfoned_cmdconv.h\
	     include/daemon/zfoned_commands.h include/daemon/zfoned_ctrl.h include/daemon/zfoned_network.h\
	     include/zfone_checker.h include/daemon/zfoned_sockets.h\
	     \
	     include/driver/zrtp_driver.h\
	     \
	     include/utils/zfone_utils.h\
		 include/utils/zrtp_poll.h\
	     \
	     include/zrtp_iface/zfone_alert.h\
	     include/zrtp_iface/zfone_cache.h \
	     include/zrtp_iface/zfone_iface.h include/zrtp_iface/zfone_log.h\
	     include/zrtp_iface/zfone_scheduler.h\
	     \
	     include/zfone.h include/zfone_cfg.h include/zfone_commands.h include/zfone_heurist.h \
	     include/zfone_manager.h include/zfone_siprocessor.h include/zfone_system.h include/zfone_types.h\
	     include/zfone_updater.h\
	     \
	     resources/sounds/amiss.pcmu resources/sounds/clear.pcmu resources/sounds/secure.pcmu resources/sounds/inactive.pcmu \
	     ./config/zfone.cfg ./config/cache.version ./scripts/unix/compare.sh ./cfg ./resources/daemon/zfone
else
SUBDIRS = src
endif

install-data-hook:
	killall zfoned || true
	killall zfone  || true
	./scripts/unix/compare.sh $(prefix)

uninstall:
	rm -rf $(prefix)/share/zfone/sounds
	rm -f $(prefix)/bin/zfoned    
	rm -f /var/log/zfone*
	./scripts/unix/delete_config.sh $(prefix)

