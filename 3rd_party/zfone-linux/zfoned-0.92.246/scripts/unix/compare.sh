#!/bin/sh

start-stop-daemon 2> /dev/null
[ "$?" = 127 ] || cp ./resources/daemon/zfone /etc/init.d/

change_config=1
delete_cache=1

MINIMAL_CONFIG_VERSION="9";
MINIMAL_CONFIG_BUILD="228";
MINIMAL_CACHE_VERSION="9";
MINIMAL_CACHE_BUILD="232";

config_file="$1/share/zfone/zfone.cfg"
cache_ver_file="$1/share/zfone/cache.version"

if [ -e "$config_file" ]; then
	old_version=`awk '$1 == "#" && $2 == "ZfoneVersion" {print $3}' "$config_file"`;
	is_ec=`awk '$1 == "#" && $2 == "ZfoneVersion" {print $4}' "$config_file"`;
	is_lec=`awk '$1 == "#" && $2 == "ZfoneVersion" {print $4}' ./config/zfone.cfg`;

	if [ "$is_ec" = "$is_lec" ]; then
	    if [ ! -z '$old_version' ]; then
			version=`echo $old_version | awk '{split($1,t,".");$1=t[2];print}'`
			build=`echo $old_version | awk '{split($1,t,".");$1=t[3];print}'`

			if [ "$version" -gt "$MINIMAL_CONFIG_VERSION" ]; then
				change_config=0;
	  		else
				if [ "$version" -eq "$MINIMAL_CONFIG_VERSION" ]; then
					if [ "$build" -ge "$MINIMAL_CONFIG_BUILD" ]; then
						change_config=0;
					fi
				fi
			fi
		fi
	fi
fi

if [ -e "$cache_ver_file" ]; then
	old_version=`awk '$1 == "ZfoneCacheVersion" {print $2}' "$cache_ver_file"`;
	if [ ! -z '$old_version' ]; then
		version=`echo $old_version | awk '{split($1,t,".");$1=t[2];print}'`
		build=`echo $old_version | awk '{split($1,t,".");$1=t[3];print}'`

		if [ "$version" -gt "$MINIMAL_CACHE_VERSION" ]; then
			delete_cache=0;
		else
			if [ "$version" -eq "$MINIMAL_CACHE_VERSION" ]; then
				if [ "$build" -ge "$MINIMAL_CACHE_BUILD" ]; then
					delete_cache=0;
				fi
			fi
		fi
	fi
fi



echo "=========================================================================="
echo "binaries  should be installed into $1/bin"
echo "resources should be installed into $1/share/zfone"
if [ $change_config = 1 ]; then
	`rm -f "$1/share/zfone/zfone.cfg" 2> /dev/null`
	`rm -f "$1/share/zfone/zfone.cfg.bak" 2> /dev/null`
	`cp ./config/zfone.cfg "$1/share/zfone" 2> /dev/null`
	echo "configuration was changed"
fi

if [ $delete_cache = 1 ]; then
	`rm -f "$1/share/zfone/zfone_cache.dat" 2> /dev/null`
	`rm -f "$1/share/zfone/zfone_zid.dat" 2> /dev/null`
	`cp ./config/cache.version "$1/share/zfone" 2> /dev/null`
	echo "caches were deleted"
fi
echo "type '/etc/init.d/zfone start' in order to start zfone daemon"
echo "=========================================================================="

