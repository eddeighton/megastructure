
cmake_minimum_required( VERSION 3.1...3.16 )

#get boost
include( ${MS_ROOT_DIR}/cmake/boost_include.cmake )

#get megastructure
include( ${MS_ROOT_DIR}/cmake/core/megastructure_include.cmake )

#get the sources
set( MS_COMPONENT_API
	${MS_API_DIR}/megastructure/component.hpp
	)

set( MS_COMPONENT_SOURCES
	${MS_SRC_DIR}/component/component.cpp
    )

add_library( component STATIC
					${MS_COMPONENT_API}
					${MS_COMPONENT_SOURCES}
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
