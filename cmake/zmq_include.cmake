
cmake_minimum_required( VERSION 3.1...3.16 )

find_path( ZMQ_INSTALL_PATH spdlog PATHS ${MEGA_THIRD_PARTY_DIR}/zeromq/install/ )

set( CMAKE_PREFIX_PATH "${ZMQ_INSTALL_PATH}/CMake;${CMAKE_PREFIX_PATH}" )

find_package( ZeroMQ  )

function( link_zmq targetname )
	target_compile_definitions( ${targetname} PUBLIC ZMQ_BUILD_DRAFT_API )
    target_link_libraries( ${targetname} libzmq )
endfunction( link_zmq )
