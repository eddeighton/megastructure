

cmake_minimum_required( VERSION 3.1...3.16 )

find_path( PROTOBUF_INSTALL_PATH cmake/protobuf-targets.cmake PATHS ${MEGA_THIRD_PARTY_DIR}/protobuf/install/ REQUIRED )

set( CMAKE_PREFIX_PATH "${PROTOBUF_INSTALL_PATH}/cmake;${CMAKE_PREFIX_PATH}" )
set( Protobuf_Dir ${PROTOBUF_INSTALL_PATH}/cmake )
set( Protobuf_LIBRARIES ${PROTOBUF_INSTALL_PATH}/lib )
set( Protobuf_INCLUDE_DIR ${PROTOBUF_INSTALL_PATH}/include )
set( Protobuf_PROTOC_EXECUTABLE ${PROTOBUF_INSTALL_PATH}/bin/protoc.exe )

find_package( Protobuf REQUIRED  )

message("   --> PROTOBUF LIB: ${PROTOBUF_LIBRARIES}")
message("   --> PROTOBUF INCLUDE: ${PROTOBUF_INCLUDE_DIRS}")
message("   --> PROTOBUF VERSION: ${Protobuf_VERSION}")
message("   --> PROTOBUF Found: ${Protobuf_FOUND}")

message( "Protobuf_PROTOC_EXECUTABLE: ${Protobuf_PROTOC_EXECUTABLE}" )
if( Protobuf_PROTOC_EXECUTABLE STREQUAL "" )
message( FATAL_ERROR "Failed to find protobuffers" )
endif()

function( link_protobuf targetname )
	target_include_directories( ${targetname} PUBLIC ${PROTOBUF_INCLUDE_DIR} )
    target_link_directories( ${targetname} PUBLIC ${PROTOBUF_LIBRARIES} )
    target_link_libraries( ${targetname} PUBLIC libprotobuf )
endfunction( link_protobuf )


