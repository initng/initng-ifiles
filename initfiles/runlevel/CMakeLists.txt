SET(RUNLEVEL_FILES
	fake-default.runlevel
	single.runlevel
)

INSTALL_FILES(${SYSCONF_INSTALL_DIR}/initng/runlevel FILES ${RUNLEVEL_FILES})
