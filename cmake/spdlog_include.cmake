
cmake_minimum_required( VERSION 3.1...3.16 )

include( ${MEGA_THIRD_PARTY_DIR}/spdlog/install/lib/cmake/spdlog/spdlogConfig.cmake )

#find_package( spdlog )

#find_path( MSGPACK_INCLUDEDIR NAMES msgpack PATHS ${MEGA_THIRD_PARTY_DIR}/msgpack/install/include/ )

function( link_spdlog targetname )
	#target_include_directories( ${targetname} PUBLIC ${MSGPACK_INCLUDEDIR} )
	#target_compile_definitions( ${targetname} PUBLIC ZMQ_BUILD_DRAFT_API )
    target_link_libraries( ${targetname} spdlog::spdlog )
endfunction( link_spdlog )

