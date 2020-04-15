cmake_minimum_required(VERSION 2.8)

#get boost
include( ${MS_ROOT_DIR}/cmake/boost_include.cmake )

#get gtest
include( ${MS_ROOT_DIR}/cmake/gtest_include.cmake )

#get the unit test utils
include( ${MS_ROOT_DIR}/cmake/test_utils_include.cmake )

#get common
include( ${MS_ROOT_DIR}/cmake/common_include.cmake )

set( MS_UNIT_TEST_DIR ${MS_ROOT_DIR}/tests/unit )

set( MS_UNIT_TESTS
	 ${MS_UNIT_TEST_DIR}/basic_test.cpp
	 ${MS_UNIT_TEST_DIR}/protocol_test.cpp
	 )

add_executable( ms_tests ${MS_UNIT_TESTS})

##configure all debug builds to have 'd' postfix
set_target_properties( ms_tests PROPERTIES DEBUG_POSTFIX d )
set_target_properties( ms_tests PROPERTIES FOLDER common )

link_boost( ms_tests filesystem )
link_boost( ms_tests system )
link_boost( ms_tests date_time )
link_boost( ms_tests program_options )
link_gtest( ms_tests )
link_common( ms_tests )
link_gtest_utils( ms_tests )

target_link_libraries( ms_tests megastructurelib )
target_link_libraries( ms_tests megastructureprotocollib )

install( TARGETS ms_tests DESTINATION bin)
