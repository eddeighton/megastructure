cmake_minimum_required(VERSION 2.8)

#get boost
include( ${MEGA_ROOT_DIR}/cmake/boost_include.cmake )

#get gtest
include( ${MEGA_ROOT_DIR}/cmake/gtest_include.cmake )

#get the unit test utils
include( ${MEGA_ROOT_DIR}/cmake/test_utils_include.cmake )

#get common
include( ${MEGA_ROOT_DIR}/cmake/common_include.cmake )

set( MEGA_UNIT_TEST_DIR ${MEGA_ROOT_DIR}/tests/unit )

set( MEGA_UNIT_TESTS
	 ${MEGA_UNIT_TEST_DIR}/basic_test.cpp
	 ${MEGA_UNIT_TEST_DIR}/protocol_test.cpp
	 )

add_executable( mega_tests ${MEGA_UNIT_TESTS})

##configure all debug builds to have 'd' postfix
set_target_properties( mega_tests PROPERTIES DEBUG_POSTFIX d )
set_target_properties( mega_tests PROPERTIES FOLDER tests )

link_boost( mega_tests filesystem )
link_boost( mega_tests system )
link_boost( mega_tests date_time )
link_boost( mega_tests program_options )
link_gtest( mega_tests )
link_common( mega_tests )
link_gtest_utils( mega_tests )

target_link_libraries( mega_tests megastructurelib )
target_link_libraries( mega_tests megastructureprotocollib )

install( TARGETS mega_tests DESTINATION bin)
