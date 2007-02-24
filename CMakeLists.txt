PROJECT(initng-ifiles C)

SET(VERSION "0.1.0" CACHE STRING "Version number of the project")

IF(CMAKE_BACKWARDS_COMPATIBILITY LESS 2.2)
	MESSAGE(FATAL_ERROR "Error: Your CMake version is too old. This project requires version 2.2 or newer.")
ENDIF(CMAKE_BACKWARDS_COMPATIBILITY LESS 2.2)

SET(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
INCLUDE(InitngMacros)

# Colorize output, needs CMake > 2.3.4
SET(CMAKE_COLOR_MAKEFILE TRUE)

SET(CMAKE_INSTALL_PREFIX "" CACHE INTERNAL "")
SET(INCLUDE_INSTALL_DIR /usr/include CACHE STRING "Install location for header files.")
SET(SBIN_INSTALL_DIR /sbin CACHE STRING "Install location for binaries.")
SET(LIB_INSTALL_DIR /lib CACHE STRING "Install location for libraries.")
SET(SYSCONF_INSTALL_DIR /etc CACHE STRING "Install location for configuration files.")
SET(DATA_INSTALL_DIR /usr/share CACHE STRING "Install location for data files.")
MARK_AS_ADVANCED(INCLUDE_INSTALL_DIR)
MARK_AS_ADVANCED(SBIN_INSTALL_DIR)
MARK_AS_ADVANCED(LIB_INSTALL_DIR)
MARK_AS_ADVANCED(SYSCONF_INSTALL_DIR)
MARK_AS_ADVANCED(DATA_INSTALL_DIR)

SUBDIRS(doc initfiles scripts tools)

OPTION(INSTALL_VIM "Install vim syntax files" ON)
OPTION(INSTALL_FIXES "Install modified system scripts adapted for initng" OFF)
OPTION(INSTALL_SCRIPTS_NET "Install network helper scripts" ON)
OPTION(INSTALL_SCRIPTS_SPLASH "Install splash helper scripts" OFF)
OPTION(INSTALL_DEBUG "Install debug ifiles" OFF)
OPTION(INSTALL_RUNLEVELS "Generate and install system.virtual and default.runlevel" OFF)

IF(INSTALL_VIM)
	ADD_SUBDIRECTORY(vim)
ENDIF(INSTALL_VIM)

IF(INSTALL_FIXES)
	ADD_SUBDIRECTORY(fixes)
ENDIF(INSTALL_FIXES)

INSTALL_PROGRAMS(${SBIN_INSTALL_DIR} FILES ${CMAKE_CURRENT_BINARY_DIR}/genrunlevel)
FILE(GLOB GENRUNLEVEL_LIBS "libgenrunlevel/lib*")
INSTALL_PROGRAMS(${LIB_INSTALL_DIR} FILES ${GENRUNLEVEL_LIBS})

IF(INSTALL_RUNLEVELS)
	SET(RUNLEVEL_FILES
		system.virtual
		default.runlevel
	)
	GENERATE_RUNLEVEL(${RUNLEVEL_FILES})
	IF(EXISTS ${CMAKE_INSTALL_PREFIX}${SYSCONF_INSTALL_DIR}/initng/default.runlevel)
		MESSAGE("INSTALL_RUNLEVEL is selected, this will overwrite your existing system.virtual and default.runlevel file.")
		MESSAGE("Make a backup of your existing files or run cmake with \"-DINSTALL_RUNLEVELS=OFF\"")
	ENDIF(EXISTS ${CMAKE_INSTALL_PREFIX}${SYSCONF_INSTALL_DIR}/initng/default.runlevel)
	INSTALL_FILES(${SYSCONF_INSTALL_DIR}/initng FILES ${RUNLEVEL_FILES})
ENDIF(INSTALL_RUNLEVELS)

CONFIGURE_FILE(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
        IMMEDIATE @ONLY)

CONFIGURE_FILE(
	"${CMAKE_CURRENT_SOURCE_DIR}/genrunlevel.in"
	"${CMAKE_CURRENT_BINARY_DIR}/genrunlevel"
	IMMEDIATE @ONLY)

ADD_CUSTOM_TARGET(uninstall
        "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")
