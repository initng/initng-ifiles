# NAME: Wicd
# DESCRIPTION: Wired and wireless network manager for GNU/Linux
# WWW: http://wicd.sourceforge.net/

daemon daemon/wicd {
	need = daemon/dbus;
	stdall = /dev/null;
	pid_file = /var/run/wicd/wicd.pid; # tested on gentoo
	forks;
	respawn;
	exec daemon = @/usr/bin/wicd@;
}
