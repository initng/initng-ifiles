SET(DOC_FILES
	imanual.txt
	iimanual.txt
)

SET(MAN8_FILES
	genrunlevel.8
	install_service.8
	ng-update.8
)

INSTALL_FILES(${DATA_INSTALL_DIR}/doc/initng-ifiles FILES ${DOC_FILES})
INSTALL_FILES(${DATA_INSTALL_DIR}/man/man8 FILES ${MAN8_FILES})
