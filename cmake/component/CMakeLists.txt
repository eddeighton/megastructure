
cmake_minimum_required( VERSION 3.1...3.16 )

#get boost
include( ${MS_ROOT_DIR}/cmake/boost_include.cmake )

#get megastructure
include( ${MS_ROOT_DIR}/cmake/core/megastructure_include.cmake )

#get the sources
set( COMPONENT_CMD_SOURCE_DIR ${MS_ROOT_DIR}/src/component )
file( GLOB COMPONENT_CMD_SOURCE_DIR_HEADERS ${COMPONENT_CMD_SOURCE_DIR}/*.hpp )
file( GLOB COMPONENT_CMD_SOURCE_DIR_SOURCES ${COMPONENT_CMD_SOURCE_DIR}/*.cpp )
set( COMPONENT_CMD_SOURCES ${COMPONENT_CMD_SOURCE_DIR_HEADERS} ${COMPONENT_CMD_SOURCE_DIR_SOURCES} )
source_group( src FILES ${COMPONENT_CMD_SOURCES} )

add_library( component STATIC
					${COMPONENT_CMD_SOURCES}
				)
		
set_target_properties( component PROPERTIES FOLDER log )

set_target_properties( component PROPERTIES DEBUG_POSTFIX d )

link_boost( component program_options )
link_boost( component filesystem )
link_boost( component system )
link_boost( component date_time )
#link_common( component )
#link_eventlog( component )
link_megastructure( component )

install( TARGETS component DESTINATION lib)
