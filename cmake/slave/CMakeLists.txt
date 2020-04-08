
cmake_minimum_required( VERSION 3.1...3.16 )

#get boost
include( ${MS_ROOT_DIR}/cmake/boost_include.cmake )

#get megastructure
include( ${MS_ROOT_DIR}/cmake/core/megastructure_include.cmake )

#get the sources
set( SLAVE_CMD_SOURCE_DIR ${MS_ROOT_DIR}/src/slave )
file( GLOB SLAVE_CMD_SOURCE_DIR_HEADERS ${SLAVE_CMD_SOURCE_DIR}/*.hpp )
file( GLOB SLAVE_CMD_SOURCE_DIR_SOURCES ${SLAVE_CMD_SOURCE_DIR}/*.cpp )
set( SLAVE_CMD_SOURCES ${SLAVE_CMD_SOURCE_DIR_HEADERS} ${SLAVE_CMD_SOURCE_DIR_SOURCES} )
source_group( src FILES ${SLAVE_CMD_SOURCES} )

add_executable( slave 
					${SLAVE_CMD_SOURCES}
				)
		
set_target_properties( slave PROPERTIES FOLDER log )

set_target_properties( slave PROPERTIES DEBUG_POSTFIX d )

link_boost( slave program_options )
link_boost( slave filesystem )
link_boost( slave system )
link_boost( slave date_time )
#link_common( slave )
#link_eventlog( slave )
link_megastructure( slave )

install( TARGETS slave DESTINATION bin)
