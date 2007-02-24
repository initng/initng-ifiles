INCLUDE_DIRECTORIES(/usr/include/initng)

ADD_EXECUTABLE(install_service install_service.c)
INSTALL_TARGETS(${SBIN_INSTALL_DIR} install_service)
INSTALL_PROGRAMS(${SBIN_INSTALL_DIR} ng-update)
