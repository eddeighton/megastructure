
cmake_minimum_required( VERSION 3.1...3.16 )

find_path( SPDLOG_INSTALL_PATH spdlog PATHS ${MEGA_THIRD_PARTY_DIR}/spdlog/install/lib/cmake/ )

#include( ${MEGA_THIRD_PARTY_DIR}/spdlog/install/lib/cmake/spdlog/spdlogConfig.cmake )

set( CMAKE_PREFIX_PATH "${SPDLOG_INSTALL_PATH}/spdlog;${CMAKE_PREFIX_PATH}" )
find_package( spdlog )

function( link_spdlog targetname )
	#target_include_directories( ${targetname} PUBLIC ${MSGPACK_INCLUDEDIR} )
    target_compile_definitions( ${targetname} PUBLIC SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE )
    target_link_libraries( ${targetname} spdlog::spdlog )
endfunction( link_spdlog )

