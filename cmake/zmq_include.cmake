
cmake_minimum_required( VERSION 3.1...3.16 )

find_package( ZeroMQ  )

function( link_zmq targetname )
    target_link_libraries( ${targetname} libzmq )
endfunction( link_zmq )
