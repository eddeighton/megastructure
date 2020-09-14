
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

#get the sources
set( MEGA_TEST_HOST_SOURCES
	${MEGA_SRC_DIR}/hosts/basic/basic.cpp
    )
	
add_executable( basic_host 
					${MEGA_TEST_HOST_SOURCES}
				)
		
set_target_properties( basic_host PROPERTIES FOLDER tests )
set_target_properties( basic_host PROPERTIES DEBUG_POSTFIX d )

link_boost( basic_host program_options )
link_boost( basic_host filesystem )
link_boost( basic_host system )
link_boost( basic_host date_time )
link_xsde( basic_host )
link_clang( basic_host )
link_common( basic_host )
link_eg( basic_host )

target_link_libraries( basic_host megastructurelib )
target_link_libraries( basic_host megastructureprotocollib )
target_link_libraries( basic_host componentlib )

install( TARGETS basic_host DESTINATION bin)
