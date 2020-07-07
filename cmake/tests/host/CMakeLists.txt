
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
	${MEGA_ROOT_DIR}/tests/host/host.cpp
    )
	
add_executable( test_host 
					${MEGA_TEST_HOST_SOURCES}
				)
		
set_target_properties( test_host PROPERTIES FOLDER tests )
set_target_properties( test_host PROPERTIES DEBUG_POSTFIX d )

link_boost( test_host program_options )
link_boost( test_host filesystem )
link_boost( test_host system )
link_boost( test_host date_time )
link_xsde( test_host )
link_clang( test_host )
link_common( test_host )
link_eg( test_host )

target_link_libraries( test_host megastructurelib )
target_link_libraries( test_host megastructureprotocollib )
target_link_libraries( test_host componentlib )

install( TARGETS test_host DESTINATION bin)
install( FILES ${MEGA_ROOT_DIR}/tests/host/.eg DESTINATION include/testhost )
