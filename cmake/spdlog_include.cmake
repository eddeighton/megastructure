
cmake_minimum_required( VERSION 3.1...3.16 )

include( ${MEGA_THIRD_PARTY_DIR}/spdlog/install/lib/cmake/spdlog/spdlogConfig.cmake )

function( link_spdlog targetname )
	#target_include_directories( ${targetname} PUBLIC ${MSGPACK_INCLUDEDIR} )
    target_compile_definitions( ${targetname} PUBLIC SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE )
    target_link_libraries( ${targetname} spdlog::spdlog )
endfunction( link_spdlog )

