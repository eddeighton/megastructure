
cmake_minimum_required( VERSION 3.1...3.16 )

#get boost
include( ${MS_ROOT_DIR}/cmake/boost_include.cmake )

#get common
include( ${MS_ROOT_DIR}/cmake/common_include.cmake )

#get megastructure
include( ${MS_ROOT_DIR}/cmake/core/megastructure_include.cmake )

#get the sources
set( TEST_EG_CMD_SOURCE_DIR ${MS_ROOT_DIR}/tests/host )
file( GLOB TEST_EG_CMD_SOURCE_DIR_HEADERS ${TEST_EG_CMD_SOURCE_DIR}/*.hpp )
file( GLOB TEST_EG_CMD_SOURCE_DIR_SOURCES ${TEST_EG_CMD_SOURCE_DIR}/*.cpp )
set( TEST_EG_CMD_SOURCES ${TEST_EG_CMD_SOURCE_DIR_HEADERS} ${TEST_EG_CMD_SOURCE_DIR_SOURCES} )
source_group( src FILES ${TEST_EG_CMD_SOURCES} )

add_library( test_eg SHARED
					${TEST_EG_CMD_SOURCES}
				)
		
set_target_properties( test_eg PROPERTIES FOLDER log )

set_target_properties( test_eg PROPERTIES DEBUG_POSTFIX d )

target_compile_definitions( test_eg PRIVATE -DMEGASTRUCTURE_COMPONENT )

link_boost( test_eg program_options )
link_boost( test_eg filesystem )
link_boost( test_eg system )
link_boost( test_eg date_time )
link_common( test_eg )
#link_eventlog( master )
link_megastructure( test_eg )

install( TARGETS test_eg DESTINATION bin)
