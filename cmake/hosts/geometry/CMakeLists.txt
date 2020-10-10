
cmake_minimum_required( VERSION 3.1...3.16 )

#get boost
include( ${MEGA_ROOT_DIR}/cmake/boost_include.cmake )

#get xsde
include( ${MEGA_ROOT_DIR}/cmake/xsde_include.cmake )

#get common
include( ${MEGA_ROOT_DIR}/cmake/common_include.cmake )

#eg
include( ${MEGA_ROOT_DIR}/cmake/eg_include.cmake )

#get the sources
set( MEGA_GEOMETRY_HOST_SOURCES
	${MEGA_SRC_DIR}/hosts/geometry/geometry_host.cpp
    )
    
set( MEGA_GEOMETRY_HOST_HEADERS
	${MEGA_SRC_DIR}/hosts/geometry/config_conversions.hpp
    )
	
add_executable( geometry_host 
					${MEGA_GEOMETRY_HOST_SOURCES}
				)
		
set_target_properties( geometry_host PROPERTIES FOLDER tests )
set_target_properties( geometry_host PROPERTIES DEBUG_POSTFIX d )

link_boost( geometry_host program_options )
link_boost( geometry_host filesystem )
link_boost( geometry_host system )
link_boost( geometry_host date_time )
link_xsde( geometry_host )
link_common( geometry_host )
link_eg( geometry_host )

target_link_libraries( geometry_host megastructurelib )
target_link_libraries( geometry_host megastructureprotocollib )
target_link_libraries( geometry_host componentlib )

install( TARGETS geometry_host DESTINATION bin)
install( FILES ${MEGA_GEOMETRY_HOST_HEADERS} DESTINATION include/geometry ) 