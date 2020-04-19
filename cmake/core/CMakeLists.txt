
cmake_minimum_required( VERSION 3.1...3.16 )

#get boost
include( ${MEGA_ROOT_DIR}/cmake/boost_include.cmake )

#zeromq
include( ${MEGA_ROOT_DIR}/cmake/zmq_include.cmake )

#common
include( ${MEGA_ROOT_DIR}/cmake/common_include.cmake )

set( MEGA_LIBRARY
	${MEGA_API_DIR}/megastructure/coordinator.hpp
	${MEGA_API_DIR}/megastructure/clientServer.hpp
	${MEGA_API_DIR}/megastructure/queue.hpp
	)

set( MEGA_SOURCES
	${MEGA_SRC_DIR}/core/coordinator.cpp
	${MEGA_SRC_DIR}/core/clientServer.cpp
	${MEGA_SRC_DIR}/core/queue.cpp
    )
    
add_library( megastructurelib 
                    ${MEGA_LIBRARY}
                    ${MEGA_SOURCES}
				)

set_target_properties( megastructurelib PROPERTIES FOLDER eg )

set_target_properties( megastructurelib PROPERTIES DEBUG_POSTFIX d )

link_boost( megastructurelib filesystem )
link_boost( megastructurelib iostreams )
link_boost( megastructurelib serialization )
link_zmq( megastructurelib )
link_common( megastructurelib )

target_link_libraries( megastructurelib megastructureprotocollib )

install( FILES ${MEGA_LIBRARY} DESTINATION include/megastructure )
install( TARGETS megastructurelib DESTINATION lib)