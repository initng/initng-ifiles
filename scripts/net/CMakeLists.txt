SET(SCRIPT_FILES
	dhclient-wrapper
	dhcp
	dhcpcd-backgrounder
	essidnet
	functions
	gentoo-functions
	ifconfig
	interface
	iproute2
	iwconfig
	system
	udhcpc-wrapper
	wpa_cli.action
	wpa_supplicant
)

INSTALL_PROGRAMS(${LIB_INSTALL_DIR}/initng/scripts/net FILES ${SCRIPT_FILES})
