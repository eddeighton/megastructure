
cmake_minimum_required( VERSION 3.1...3.16 )

#get boost
include( ${MEGA_ROOT_DIR}/cmake/boost_include.cmake )

#get xsde
include( ${MEGA_ROOT_DIR}/cmake/xsde_include.cmake )

#get clang
include( ${MEGA_ROOT_DIR}/cmake/clang_include.cmake )

#get mega schema
include( ${MEGA_ROOT_DIR}/cmake/schema/schema_include.cmake )

#get common
include( ${MEGA_ROOT_DIR}/cmake/common_include.cmake )

#eg
include( ${MEGA_ROOT_DIR}/cmake/eg_include.cmake )

#get megastructure
include( ${MEGA_ROOT_DIR}/cmake/core/megastructure_include.cmake )

#get the sources
set( MEGA_SLAVE_SOURCES
	${MEGA_SRC_DIR}/slave/slave.hpp
	${MEGA_SRC_DIR}/slave/slave.cpp
	${MEGA_SRC_DIR}/slave/hostMap.hpp
	${MEGA_SRC_DIR}/slave/hostMap.cpp
	${MEGA_SRC_DIR}/slave/activitiesMaster.hpp
	${MEGA_SRC_DIR}/slave/activitiesMaster.cpp
	${MEGA_SRC_DIR}/slave/activitiesHost.hpp
	${MEGA_SRC_DIR}/slave/activitiesHost.cpp
	${MEGA_SRC_DIR}/slave/activitiesEG.hpp
	${MEGA_SRC_DIR}/slave/activitiesEG.cpp
	${MEGA_SRC_DIR}/slave/main.cpp
    )
	
add_executable( slave 
					${MEGA_SLAVE_SOURCES}
				)
		
set_target_properties( slave PROPERTIES FOLDER core )
set_target_properties( slave PROPERTIES DEBUG_POSTFIX d )

link_boost( slave program_options )
link_boost( slave filesystem )
link_boost( slave system )
link_boost( slave date_time )
link_boost( slave fiber )
link_xsde( slave )
link_clang( slave )
link_schemalib( slave )
link_common( slave )
link_eg( slave )
link_megastructure( slave )

install( TARGETS slave DESTINATION bin)
