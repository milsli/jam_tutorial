cmake_minimum_required (VERSION 2.6)
project (jam_tutorials)

SET( CMAKE_DEBUG_POSTFIX  "_d" )
SET( CMAKE_STANDARD_LIBRARIES "" )
SET( CMAKE_SHARED_MODULE_PREFIX "" )
SET( DEBUG_POSTFIX "_d" )

IF( ${CMAKE_BUILD_TYPE} MATCHES "Debug" )
	ADD_DEFINITIONS("-D_DEBUG")
ENDIF()  

INCLUDE_DIRECTORIES(
  ${CMAKE_SOURCE_DIR}/jam/include
  ${CMAKE_SOURCE_DIR}/jam_tutorials/include
)

MACRO(LINK_INTERNAL TRGTNAME)
    FOREACH(LINKLIB ${ARGN})
        TARGET_LINK_LIBRARIES(${TRGTNAME} ${LINKLIB} )
    ENDFOREACH(LINKLIB)
ENDMACRO(LINK_INTERNAL TRGTNAME)

MACRO(LINK_EXTERNAL TRGTNAME)
    FOREACH(LINKLIB ${ARGN})
	IF( EXISTS( "${LINKLIB}d" ) ) 
        	TARGET_LINK_LIBRARIES(${TRGTNAME} "${LINKLIB}" debug "${LINKLIB}d" )
	ELSEIF( EXISTS( "${LINKLIB}_d" ) ) 
        	TARGET_LINK_LIBRARIES(${TRGTNAME} "${LINKLIB}" debug "${LINKLIB}_d" )
	ELSE()
        	TARGET_LINK_LIBRARIES(${TRGTNAME} "${LINKLIB}" )
	ENDIF()

    ENDFOREACH(LINKLIB)
ENDMACRO(LINK_EXTERNAL TRGTNAME)

MACRO(BUILD_TARGET TARGET_NAME TARGET_TYPE INTERNAL_LIBS EXTERNAL_LIBS HEADER_PATH )
	FILE(GLOB cpp_files *.cpp)
	FILE(GLOB h_files ${HEADER_PATH} )
#	MESSAGE( "MODULE: "${TARGET_NAME} )
#	MESSAGE( "  HFILES: ""${h_files}" )
#	MESSAGE( "  CPPFILES: ""${cpp_files}" )
#	MESSAGE( "  EXTERNAL: ""${EXTERNAL_LIBS}" )
#	MESSAGE( "  INTERNAL: ""${INTERNAL_LIBS}" )	

IF( ${TARGET_TYPE} STREQUAL "EXECUTABLE" )
	#SET( EXECUTABLE_OUTPUT_PATH ./../../bin/ )
	ADD_EXECUTABLE( ${TARGET_NAME} ${cpp_files} ${h_files} )	
	# CMAKE_DEBUG_POSTFIX works only for libs. Lets make sure exe is suffixed as well
	SET_TARGET_PROPERTIES( ${TARGET_NAME} PROPERTIES DEBUG_POSTFIX ${DEBUG_POSTFIX} )	
ELSE()
	IF( "${TARGET_TYPE}" STREQUAL "STATIC" )
		#SET( LIBRARY_OUTPUT_PATH ./../../lib/ )
	ELSE()
		#SET( LIBRARY_OUTPUT_PATH ./../../bin/ )
	ENDIF()
	ADD_LIBRARY( ${TARGET_NAME} ${TARGET_TYPE} ${h_files} ${cpp_files} )
ENDIF()
	
LINK_INTERNAL( ${TARGET_NAME} "${INTERNAL_LIBS}" )
LINK_EXTERNAL( ${TARGET_NAME} "${EXTERNAL_LIBS}" )

INSTALL(TARGETS ${TARGET_NAME}
		RUNTIME DESTINATION bin
		LIBRARY DESTINATION bin
	 	ARCHIVE DESTINATION lib )

ENDMACRO()

add_subdirectory (./jam/src/jam_core)
add_subdirectory (./jam/src/jam_app)
add_subdirectory (./jam_tutorials/src/tutorial1_hello_world)
add_subdirectory (./jam_tutorials/src/tutorial2_player_xo)
add_subdirectory (./jam_tutorials/src/tutorial2_referee_xo)

install(FILES ./jam_tutorials/cfg/tutorial1.cfg DESTINATION bin )
install(FILES ./jam_tutorials/cfg/tutorial2.cfg DESTINATION bin )

