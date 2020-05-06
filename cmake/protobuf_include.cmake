

cmake_minimum_required( VERSION 3.1...3.16 )

include( ${MEGA_THIRD_PARTY_DIR}/protobuf/install/cmake/protobuf-targets.cmake )

find_package( Protobuf REQUIRED  )

#message("   --> PROTOBUF LIB: ${PROTOBUF_LIBRARIES}")
#message("   --> PROTOBUF INCLUDE: ${PROTOBUF_INCLUDE_DIRS}")
#message("   --> PROTOBUF VERSION: ${Protobuf_VERSION}")
#message("   --> PROTOBUF Found: ${Protobuf_FOUND}")

function( link_protobuf targetname )
	target_include_directories( ${targetname} PUBLIC ${PROTOBUF_INCLUDE_DIR} )
    target_link_libraries( ${targetname} PUBLIC ${PROTOBUF_LIBRARIES} )
endfunction( link_protobuf )


