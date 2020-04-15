cmake_minimum_required(VERSION 2.8)

find_path( GTEST_UTILS_INSTALL_PATH share/gtest_utils-config.cmake PATHS ${CMAKE_CURRENT_LIST_DIR}../../../gtest_utils/install )

include( ${GTEST_UTILS_INSTALL_PATH}/share/gtest_utils-config.cmake )

find_package( gtest_utils REQUIRED )

function( link_gtest_utils targetname )
	target_link_libraries( ${targetname} GTestUtils::gtest_utilslib )
endfunction( link_gtest_utils )
