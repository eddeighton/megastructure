
cmake_minimum_required( VERSION 3.1...3.16 )

#get boost
include( ${MEGA_ROOT_DIR}/cmake/boost_include.cmake )

#get megastructure
include( ${MEGA_ROOT_DIR}/cmake/core/megastructure_include.cmake )

#get the sources
set( MEGA_COMPONENT_API
	${MEGA_API_DIR}/megastructure/component.hpp
	)

set( MEGA_COMPONENT_SOURCES
	${MEGA_SRC_DIR}/component/component.cpp
    )

add_library( componentlib STATIC
					${MEGA_COMPONENT_API}
					${MEGA_COMPONENT_SOURCES}
				)
		
set_target_properties( componentlib PROPERTIES FOLDER log )

set_target_properties( componentlib PROPERTIES DEBUG_POSTFIX d )

link_boost( componentlib program_options )
link_boost( componentlib filesystem )
link_boost( componentlib system )
link_boost( componentlib date_time )
#link_common( componentlib )
#link_eventlog( componentlib )
link_megastructure( componentlib )

install( TARGETS componentlib DESTINATION lib)
