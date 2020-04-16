
cmake_minimum_required( VERSION 3.1...3.16 )

#get boost
include( ${MS_ROOT_DIR}/cmake/boost_include.cmake )

#get megastructure
include( ${MS_ROOT_DIR}/cmake/core/megastructure_include.cmake )

#get the sources
set( MS_MASTER_SOURCES
	${MS_SRC_DIR}/master/master.cpp
    )
	
add_executable( master 
					${MS_MASTER_SOURCES}
				)
		
set_target_properties( master PROPERTIES FOLDER log )

set_target_properties( master PROPERTIES DEBUG_POSTFIX d )

link_boost( master program_options )
link_boost( master filesystem )
link_boost( master system )
link_boost( master date_time )
#link_common( master )
#link_eventlog( master )
link_megastructure( master )

install( TARGETS master DESTINATION bin)
