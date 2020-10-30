cmake_minimum_required(VERSION 2.8)


#get boost
include( ${MEGA_ROOT_DIR}/cmake/boost_include.cmake )

#get gtest
include( ${MEGA_ROOT_DIR}/cmake/gtest_include.cmake )

#get message pack
include( ${MEGA_ROOT_DIR}/cmake/messagepack_include.cmake )

#get spdlog
include( ${MEGA_ROOT_DIR}/cmake/spdlog_include.cmake )

#get common
include( ${MEGA_ROOT_DIR}/cmake/common_include.cmake )

set( MEGA_UNIT_TEST_DIR ${MEGA_ROOT_DIR}/tests/unit )

set( MEGA_UNIT_TESTS
    ${MEGA_UNIT_TEST_DIR}/basic_test.cpp
    ${MEGA_UNIT_TEST_DIR}/protocol_test.cpp
    ${MEGA_UNIT_TEST_DIR}/message_pack_tests.cpp
    ${MEGA_UNIT_TEST_DIR}/spdlog_test.cpp
    ${MEGA_UNIT_TEST_DIR}/build_tools.cpp
    ${MEGA_UNIT_TEST_DIR}/identifier_parser_test.cpp
    )

add_executable( mega_tests ${MEGA_UNIT_TESTS})

##configure all debug builds to have 'd' postfix
set_target_properties( mega_tests PROPERTIES DEBUG_POSTFIX d )
set_target_properties( mega_tests PROPERTIES FOLDER tests )

link_boost( mega_tests filesystem )
link_boost( mega_tests system )
link_boost( mega_tests program_options )
link_boost( mega_tests date_time )
link_gtest( mega_tests )
link_messagePack( mega_tests )
link_spdlog( mega_tests )
link_common( mega_tests )
link_gtest( mega_tests )
link_gtest_main( mega_tests )

target_link_libraries( mega_tests megastructurelib )
target_link_libraries( mega_tests megastructureprotocollib )

install( TARGETS mega_tests DESTINATION bin)
