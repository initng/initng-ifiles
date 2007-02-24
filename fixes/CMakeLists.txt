SET(SCRIPT_FILES
	ifplugd.action
	wpa_cli.action
	ngcupdown
)
INSTALL_PROGRAMS(/usr/sbin FILES ${SCRIPT_FILES})
INSTALL_PROGRAMS(${SYSCONF_INSTALL_DIR}/hotplug FILES net.agent)
INSTALL_PROGRAMS(${SYSCONF_INSTALL_DIR}/pcmcia FILES network)
INSTALL_PROGRAMS(${SYSCONF_INSTALL_DIR}/ifplugd/action.d FILES ngcupdown)
