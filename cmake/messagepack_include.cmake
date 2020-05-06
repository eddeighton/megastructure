

cmake_minimum_required( VERSION 3.1...3.16 )

#include( ${MEGA_THIRD_PARTY_DIR}/msgpack/lib/cmake/msgpack/msgpack-targets.cmake )

#find_package( MessagePack )

find_path( MSGPACK_INCLUDEDIR NAMES msgpack PATHS ${MEGA_THIRD_PARTY_DIR}/msgpack/install/include/ )

function( link_messagePack targetname )
	target_include_directories( ${targetname} PUBLIC ${MSGPACK_INCLUDEDIR} )
	#target_compile_definitions( ${targetname} PUBLIC ZMQ_BUILD_DRAFT_API )
    #target_link_libraries( ${targetname} messagepack )
endfunction( link_messagePack )
