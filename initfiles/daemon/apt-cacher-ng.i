# NAME: apt-cacher-ng
# DESCRIPTION: Apt-Cacher NG package proxy
# WWW:

daemon daemon/apt-cacher-ng {
        need = system/bootmisc virtual/net;
        suid = apt-cacher-ng;
        sgid = apt-cacher-ng;
        env RUNDIR = /var/run/apt-cacher-ng;
        env PIDFILE = ${RUNDIR}/pid;
        env SOCKET = ${RUNDIR}/socket;
        exec daemon = /usr/sbin/apt-cacher-ng -c /etc/apt-cacher-ng
                      pidfile=${PIDFILE} SocketPath=${SOCKET} foreground=1;
        script kill = {
                killall apt-cacher-ng
                rm -f ${RUNDIR}/pid ${RUNDIR}/socket
        };
}
