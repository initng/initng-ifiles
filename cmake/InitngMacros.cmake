MACRO(PROCESS_IIFILES _i_FILES)
	# reset the variable
	SET(${_i_FILES})
	SET(_ii_IGNORE)

	FOREACH(_current_FILE ${ARGN})
		GET_FILENAME_COMPONENT(_basename ${_current_FILE} NAME_WE)

		# process files only once
		IF(NOT _ii_IGNORE MATCHES /${_basename}\\.i;)
			GET_FILENAME_COMPONENT(_abs_PATH ${_current_FILE} PATH)
			SET(_i_FILE ${CMAKE_CURRENT_BINARY_DIR}/${_basename}.i)

			ADD_CUSTOM_COMMAND(OUTPUT ${_i_FILE}
				COMMAND ${CMAKE_BINARY_DIR}/tools/install_service
				ARGS -i ${_current_FILE} -o ${_i_FILE} > /dev/null 2>&1
				DEPENDS ${_current_FILE} ${CMAKE_BINARY_DIR}/tools/install_service)

			SET(${_i_FILES} ${${_i_FILES}} ${_i_FILE})

			# add to the list of processed files
			SET(_ii_IGNORE ${_ii_IGNORE} /${_basename}.i)
		ENDIF(NOT _ii_IGNORE MATCHES /${_basename}\\.i;)
	ENDFOREACH(_current_FILE)

	ADD_CUSTOM_TARGET(ifiles ALL DEPENDS ${${_i_FILES}})

ENDMACRO(PROCESS_IIFILES)

MACRO(GENERATE_RUNLEVEL)
	SET(_runlevel_FILES)
	FOREACH(_current_FILE ${ARGN})
		SET(_runlevel_FILE ${CMAKE_CURRENT_BINARY_DIR}/${_current_FILE})
		ADD_CUSTOM_COMMAND(OUTPUT ${_runlevel_FILE}
			COMMAND ${CMAKE_BINARY_DIR}/genrunlevel
			ARGS -confdir ${CMAKE_CURRENT_BINARY_DIR} ${_current_FILE} > /dev/null 2>&1
			DEPENDS ${CMAKE_SOURCE_DIR}/genrunlevel.in)

		SET(_runlevel_FILES ${_runlevel_FILES} ${_runlevel_FILE})

	ENDFOREACH(_current_FILE)

	ADD_CUSTOM_TARGET(runlevel ALL
		DEPENDS ${_runlevel_FILES})

ENDMACRO(GENERATE_RUNLEVEL)
