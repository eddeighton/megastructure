
cmake_minimum_required( VERSION 3.1...3.16 )

#get boost
include( ${MEGA_ROOT_DIR}/cmake/boost_include.cmake )

#get clang
include( ${MEGA_ROOT_DIR}/cmake/clang_include.cmake )

#get xsde
include( ${MEGA_ROOT_DIR}/cmake/xsde_include.cmake )

#get common
include( ${MEGA_ROOT_DIR}/cmake/common_include.cmake )

#eg
include( ${MEGA_ROOT_DIR}/cmake/eg_include.cmake )

#get megastructure
include( ${MEGA_ROOT_DIR}/cmake/core/megastructure_include.cmake )

#get the sources
set( MEGA_COMPONENT_API
	${MEGA_API_DIR}/megastructure/component.hpp
	${MEGA_API_DIR}/megastructure/program.hpp
	)

set( MEGA_COMPONENT_SOURCES
	${MEGA_SRC_DIR}/component/component.cpp
	${MEGA_SRC_DIR}/component/activities.hpp
	${MEGA_SRC_DIR}/component/activities.cpp
	${MEGA_SRC_DIR}/component/jobs.hpp
	${MEGA_SRC_DIR}/component/jobs.cpp
	${MEGA_SRC_DIR}/component/program.cpp
    )

add_library( componentlib STATIC
					${MEGA_COMPONENT_API}
					${MEGA_COMPONENT_SOURCES}
				)
		
set_target_properties( componentlib PROPERTIES FOLDER core )
set_target_properties( componentlib PROPERTIES DEBUG_POSTFIX d )
target_link_options( componentlib PUBLIC "-SAFESEH:NO" )

link_boost( componentlib program_options )
link_boost( componentlib filesystem )
link_boost( componentlib system )
link_boost( componentlib date_time )
link_xsde( componentlib )
link_clang( componentlib )
link_common( componentlib )
link_eg( componentlib )
#link_common( componentlib )
#link_eventlog( componentlib )
link_megastructure( componentlib )

install( TARGETS componentlib DESTINATION lib)
