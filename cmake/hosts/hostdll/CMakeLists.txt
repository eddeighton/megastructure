
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
	${MEGA_API_DIR}/hostdll/hostdll.hpp
    )
    
    
#get the sources
set( HOST_SOURCES
	${MEGA_SRC_DIR}/hosts/hostdll/hostdll.cpp
    )
	
add_library( hostdll SHARED
					${HOST_API}
					${HOST_SOURCES}
				)
		
#set_target_properties( hostdll PROPERTIES PUBLIC_HEADER "${HOST_API}" )
set_target_properties( hostdll PROPERTIES FOLDER hostdll )
set_target_properties( hostdll PROPERTIES DEBUG_POSTFIX d )

target_compile_definitions( hostdll PRIVATE IS_MEGAHOST_COMPONENT )

link_boost( hostdll filesystem )
link_boost( hostdll system )
link_boost( hostdll date_time )
link_xsde( hostdll )
link_clang( hostdll )
link_spdlog( hostdll )
link_schemalib( hostdll )
link_common( hostdll )
link_eg( hostdll )
link_megastructure( hostdll )

target_link_libraries( hostdll megastructurelib )
target_link_libraries( hostdll megastructureprotocollib )
target_link_libraries( hostdll componentlib )

install( TARGETS hostdll RUNTIME DESTINATION bin)
    
install( FILES ${HOST_API} DESTINATION include/hostdll )

install( TARGETS hostdll LIBRARY DESTINATION lib)
