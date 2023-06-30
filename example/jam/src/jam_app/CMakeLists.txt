SET( INT_LIBS jamcore )
IF(WIN32)
	SET( EXT_LIBS user32 winmm)
ELSE(WIN32)
	SET( EXT_LIBS pthread ) # needed for modules using multithreading in unix
ENDIF(WIN32)
BUILD_TARGET( jamapp EXECUTABLE "${INT_LIBS}" "${EXT_LIBS}" *.h )
