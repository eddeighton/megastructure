
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
	${MEGA_ROOT_DIR}/tests/hostdll/hostdll.cpp
    )
	
add_executable( test_host_dll 
					${MEGA_TEST_HOST_SOURCES}
				)
		
set_target_properties( test_host_dll PROPERTIES FOLDER tests )
set_target_properties( test_host_dll PROPERTIES DEBUG_POSTFIX d )

link_boost( test_host_dll program_options )
link_boost( test_host_dll filesystem )
link_boost( test_host_dll system )
link_boost( test_host_dll date_time )
link_xsde( test_host_dll )
link_clang( test_host_dll )
link_common( test_host_dll )
link_eg( test_host_dll )

target_link_libraries( test_host_dll megastructurelib )
target_link_libraries( test_host_dll megastructureprotocollib )
target_link_libraries( test_host_dll componentlib )
target_link_libraries( test_host_dll host )

install( TARGETS test_host_dll DESTINATION bin)
install( FILES ${MEGA_ROOT_DIR}/tests/host/.eg DESTINATION include/testhost )
