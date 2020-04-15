

cmake_minimum_required( VERSION 3.1...3.16 )

include( ${MS_THIRD_PARTY_DIR}/protobuf/install/cmake/protobuf-targets.cmake )

find_package( Protobuf REQUIRED  )

function( link_protobuf targetname )
	target_include_directories( ${targetname} PUBLIC ${PROTOBUF_INCLUDE_DIR} )
    target_link_libraries( ${targetname} PUBLIC ${PROTOBUF_LIBRARIES} )
endfunction( link_protobuf )


