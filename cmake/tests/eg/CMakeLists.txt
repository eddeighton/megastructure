
cmake_minimum_required( VERSION 3.1...3.16 )

#get boost
include( ${MEGA_ROOT_DIR}/cmake/boost_include.cmake )

#get common
include( ${MEGA_ROOT_DIR}/cmake/common_include.cmake )

#eg
include( ${MEGA_ROOT_DIR}/cmake/eg_include.cmake )

#get the sources
set( MEGA_TEST_EG_SOURCES
	${MEGA_ROOT_DIR}/tests/eg/test_eg.cpp
    )
	
add_library( test_eg SHARED
					${MEGA_TEST_EG_SOURCES}
				)
		
set_target_properties( test_eg PROPERTIES FOLDER tests )
set_target_properties( test_eg PROPERTIES DEBUG_POSTFIX d )

target_compile_definitions( test_eg PRIVATE -DMEGASTRUCTURE_COMPONENT )

link_boost( test_eg program_options )
link_boost( test_eg filesystem )
link_boost( test_eg system )
link_boost( test_eg date_time )
link_common( test_eg )
link_eg( test_eg )

target_link_libraries( test_eg megastructurelib )
target_link_libraries( test_eg megastructureprotocollib )

install( TARGETS test_eg DESTINATION bin)
