
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
	${MEGA_ROOT_DIR}/tests/hostdll_runner/hostdll_runner.cpp
    )
	
add_executable( hostdll_runner 
					${MEGA_TEST_HOST_SOURCES}
				)
		
set_target_properties( hostdll_runner PROPERTIES FOLDER tests )
set_target_properties( hostdll_runner PROPERTIES DEBUG_POSTFIX d )

link_boost( hostdll_runner program_options )
link_boost( hostdll_runner filesystem )
link_boost( hostdll_runner system )
link_boost( hostdll_runner date_time )
link_xsde( hostdll_runner )
link_clang( hostdll_runner )
link_common( hostdll_runner )
link_eg( hostdll_runner )

target_link_libraries( hostdll_runner megastructurelib )
target_link_libraries( hostdll_runner megastructureprotocollib )
target_link_libraries( hostdll_runner componentlib )
target_link_libraries( hostdll_runner hostdll )

install( TARGETS hostdll_runner DESTINATION bin)
