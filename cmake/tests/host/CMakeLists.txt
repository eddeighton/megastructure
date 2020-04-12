
cmake_minimum_required( VERSION 3.1...3.16 )

#get boost
include( ${MS_ROOT_DIR}/cmake/boost_include.cmake )

#get common
include( ${MS_ROOT_DIR}/cmake/common_include.cmake )

#get megastructure
include( ${MS_ROOT_DIR}/cmake/core/megastructure_include.cmake )

#get the sources
set( TEST_HOST_CMD_SOURCE_DIR ${MS_ROOT_DIR}/tests/host )
file( GLOB TEST_HOST_CMD_SOURCE_DIR_HEADERS ${TEST_HOST_CMD_SOURCE_DIR}/*.hpp )
file( GLOB TEST_HOST_CMD_SOURCE_DIR_SOURCES ${TEST_HOST_CMD_SOURCE_DIR}/*.cpp )
set( TEST_HOST_CMD_SOURCES ${TEST_HOST_CMD_SOURCE_DIR_HEADERS} ${TEST_HOST_CMD_SOURCE_DIR_SOURCES} )
source_group( src FILES ${TEST_HOST_CMD_SOURCES} )

add_executable( test_host 
					${TEST_HOST_CMD_SOURCES}
				)
		
set_target_properties( test_host PROPERTIES FOLDER log )

set_target_properties( test_host PROPERTIES DEBUG_POSTFIX d )

link_boost( test_host program_options )
link_boost( test_host filesystem )
link_boost( test_host system )
link_boost( test_host date_time )
link_common( test_host )
#link_eventlog( master )
link_megastructure( test_host )

install( TARGETS test_host DESTINATION bin)
