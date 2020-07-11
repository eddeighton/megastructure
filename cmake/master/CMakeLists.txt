
cmake_minimum_required( VERSION 3.1...3.16 )

#get boost
include( ${MEGA_ROOT_DIR}/cmake/boost_include.cmake )

#get xsde
include( ${MEGA_ROOT_DIR}/cmake/xsde_include.cmake )

#get clang
include( ${MEGA_ROOT_DIR}/cmake/clang_include.cmake )

#get spdlog
include( ${MEGA_ROOT_DIR}/cmake/spdlog_include.cmake )

#get mega schema
include( ${MEGA_ROOT_DIR}/cmake/schema/schema_include.cmake )

#get common
include( ${MEGA_ROOT_DIR}/cmake/common_include.cmake )

#eg
include( ${MEGA_ROOT_DIR}/cmake/eg_include.cmake )

#get megastructure
include( ${MEGA_ROOT_DIR}/cmake/core/megastructure_include.cmake )

#get the sources
set( MEGA_MASTER_SOURCES
	${MEGA_SRC_DIR}/master/master.cpp
	${MEGA_SRC_DIR}/master/master.hpp
	${MEGA_SRC_DIR}/master/main.cpp
	${MEGA_SRC_DIR}/master/activities.hpp
	${MEGA_SRC_DIR}/master/activities.cpp
    )
	
add_executable( master 
					${MEGA_MASTER_SOURCES}
				)
		
set_target_properties( master PROPERTIES FOLDER core )
set_target_properties( master PROPERTIES DEBUG_POSTFIX d )

link_boost( master program_options )
link_boost( master filesystem )
link_boost( master system )
link_boost( master date_time )
link_xsde( master )
link_clang( master )
link_spdlog( master )
link_schemalib( master )
link_common( master )
link_eg( master )
link_megastructure( master )

install( TARGETS master DESTINATION bin)
