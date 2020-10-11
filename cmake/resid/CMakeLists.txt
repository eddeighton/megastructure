
cmake_minimum_required( VERSION 3.1...3.16 )

#get boost
include( ${MEGA_ROOT_DIR}/cmake/boost_include.cmake )

#get xsde
include( ${MEGA_ROOT_DIR}/cmake/xsde_include.cmake )

#get spdlog
include( ${MEGA_ROOT_DIR}/cmake/spdlog_include.cmake )

#get mega schema
include( ${MEGA_ROOT_DIR}/cmake/schema/schema_include.cmake )

#get common
include( ${MEGA_ROOT_DIR}/cmake/common_include.cmake )

#get ed
include( ${MEGA_ROOT_DIR}/cmake/ed_include.cmake )

#get blueprint
include( ${MEGA_ROOT_DIR}/cmake/blueprint_include.cmake )

#eg
include( ${MEGA_ROOT_DIR}/cmake/eg_include.cmake )

#get megastructure
include( ${MEGA_ROOT_DIR}/cmake/core/megastructure_include.cmake )

#get the sources
set( MEGA_MASTER_SOURCES
	${MEGA_SRC_DIR}/resid/main.cpp
    )
	
add_executable( resid 
					${MEGA_MASTER_SOURCES}
				)
		
set_target_properties( resid PROPERTIES FOLDER core )
set_target_properties( resid PROPERTIES DEBUG_POSTFIX d )

link_boost( resid program_options )
link_boost( resid filesystem )
link_boost( resid system )
link_boost( resid date_time )
link_xsde( resid )
link_spdlog( resid )
link_schemalib( resid )
link_common( resid )
link_ed( resid )
link_blueprint( resid )
link_eg_runtime( resid )
link_megastructure( resid )

install( TARGETS resid DESTINATION bin)
