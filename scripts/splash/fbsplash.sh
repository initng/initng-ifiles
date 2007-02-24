#!/bin/bash

#fbsplash functions for initng
#based on Spock's splash functions for gentoo

#variables:

get_initng_rl() {
	# get initng runlevel
	rl="`ps -o args= 1`"
	rl="${rl#*[}"
	rl="${rl%]}" 
	echo ${rl}
}

parse_initng_runlevel() {
	cat ${1} | while read line; do
		if [ -z `echo ${line} | grep /` ]; then
			parse_initng_runlevel "`dirname ${1}`/${line}.runlevel"
		else
			if [ "${line%/*}" == "net" ]; then
				echo -n "${line/\//.} " 
			elif [ "${line#*/?}" == "dm" ]; then
				echo -n "xdm "
			else
				echo -n "${line#*/} "
			fi
		fi
	done
}


get_paint_cmd(){
	res="`/lib/splash/bin/fbres`"
	xres="${res%x*}"
	has_text="no"

	eval `cat /etc/splash/${THEME}/${res}.cfg | grep "^text_[a-z]\+="`

	[[ -n "${text_x}" && -n "${text_y}" && -n "${text_size}" ]] && has_text="yes"
	[ ${has_text} == "yes" ] && echo ",cmd:paint rect ${text_x} ${text_y} ${xres} $(($text_y+5*$text_size))"
}	

spl_comm(){
	#TODO: check if splash_util is running
	echo "$*" > /lib/splash/cache/.splash
}

parse_kcmdline(){
	[ -z "$(grep 'splash=' /proc/cmdline)" ] && exit 0
	[ ! -e "/dev/fbsplash" ] && exit 0
	SPLASHOPTS="$(sed -e 's/.*splash=//g' -e 's/ .*//g' < /proc/cmdline)"
	MODE="verbose"
	[ ! -z "$(echo ${SPLASHOPTS} | grep silent)" ] && MODE="silent"
	THEME="$(echo ${SPLASHOPTS} | sed -e 's/.*theme://g' -e 's/,.*//g' )"
}

stop_splash(){
	killall -9 splash_util 2> /dev/null
	ngc -S off
	parse_kcmdline
#	[ -x "/etc/splash/${THEME}/scripts/rc_exit-post" ] && /etc/splash/${THEME}/scripts/rc_exit-post
	
	splash_util -c setcfg -t ${THEME} --tty=1
	splash_util -c setpic -t ${THEME} --tty=1
	splash_util -c on --tty=1
}

start_splash(){
	#kill daemon (if running) & parse cmdline & set verbose pic
	stop_splash
	
	#verbose pic has been set, so just exit
	[ "${MODE}" == "verbose" ] && exit 0


	if [ -x "/etc/splash/${THEME}/scripts/rc_init-pre" ]; then
		if ! touch /lib/splash/cache/svcs_start; then
			echo "/lib/splash/cache/ is not writable; mounting tmpfs on it"
			mount none -t tmpfs /lib/splash/cache/
			mkfifo /lib/splash/cache/.splash
		fi
		rl=`get_initng_rl`
		svcs=`parse_initng_runlevel "/etc/initng/${rl}.runlevel"`
		echo ${svcs} > /lib/splash/cache/svcs_start
	
		export RUNLEVEL=S
		export SPLASH_THEME="${THEME}"
		echo "Theme ${THEME} provides an rc_init-pre script; starting it"
		/etc/splash/${THEME}/scripts/rc_init-pre sysinit
		echo "rc_init-pre script done"
	fi

	echo Starting splash_util in Daemon mode ...

	splash_util -d -t ${THEME} --tty=1

	spl_comm "set message Initng ${1} (\$progress%) ... Press F2 for verbose mode"
	spl_comm "set mode silent"
	
#	for i in ${svcs}; do
#		spl_comm "update_svc $i svc_start"
#	done
	i=0
	ngc -s | while read line; do
		i=$(($i + 1))
		if [ $i -lt 7 ]; then
			continue
		fi
		serv="${line# }"
		serv="${serv#* }"
		serv="${serv%% *}"
		if [ "${serv%/*}" == "net" ]; then
			serv="${serv/\//.}"
		else
			serv="${serv#*/}"
		fi
		state="${line##* }"
#		echo $serv
		if [ "${state}" == "DONE" -o "${state}" == "RUNNING" ]; then
			svc_state="svc_started"
		else
			svc_state="svc_start"
		fi
		spl_comm update_svc ${serv} ${svc_state}
#		spl_comm repaint
#		sleep .1
	done
	spl_comm "repaint"

	ngc -S "gensplash,boot`get_paint_cmd`"
}

#main programm:

case $1 in
	start)
		start_splash "booting"
		;;
	shutdown)
		start_splash "shutting down"
		;;
	stop)
		stop_splash 
		;;
esac
exit 0
