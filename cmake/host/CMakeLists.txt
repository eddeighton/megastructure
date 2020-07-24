
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

#get the api
set( HOST_API
	${MEGA_API_DIR}/host/host.hpp
    )
    
    
#get the sources
set( HOST_SOURCES
	${MEGA_SRC_DIR}/host/host.cpp
    )
	
add_library( host SHARED
					${HOST_API}
					${HOST_SOURCES}
				)
		
#set_target_properties( host PROPERTIES PUBLIC_HEADER "${HOST_API}" )
set_target_properties( host PROPERTIES FOLDER host )
set_target_properties( host PROPERTIES DEBUG_POSTFIX d )

target_compile_definitions( host PRIVATE IS_MEGAHOST_COMPONENT )

link_boost( host filesystem )
link_boost( host system )
link_boost( host date_time )
link_xsde( host )
link_clang( host )
link_spdlog( host )
link_schemalib( host )
link_common( host )
link_eg( host )
link_megastructure( host )

target_link_libraries( host megastructurelib )
target_link_libraries( host megastructureprotocollib )
target_link_libraries( host componentlib )

install( TARGETS host RUNTIME DESTINATION bin)
    
install( FILES ${HOST_API} DESTINATION include/host )
install( FILES ${MEGA_API_DIR}/host/.eg DESTINATION include/host )

install( TARGETS host LIBRARY DESTINATION lib)
