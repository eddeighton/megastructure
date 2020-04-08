
cmake_minimum_required( VERSION 3.1...3.16 )

#get boost
include( ${MS_ROOT_DIR}/cmake/boost_include.cmake )

#zeromq
include( ${MS_ROOT_DIR}/cmake/zmq_include.cmake )

set( MS_LIBRARY
	${MS_API_DIR}/megastructure/coordinator.hpp
	)

set( MS_SOURCES
	${MS_SRC_DIR}/core/coordinator.cpp
    )
    
add_library( megastructurelib 
                    ${MS_LIBRARY}
                    ${MS_SOURCES}
				)

set_target_properties( megastructurelib PROPERTIES FOLDER eg )

set_target_properties( megastructurelib PROPERTIES DEBUG_POSTFIX d )

link_boost( megastructurelib filesystem )
link_boost( megastructurelib iostreams )
link_boost( megastructurelib serialization )
link_zmq( megastructurelib )

install( FILES ${MS_LIBRARY} DESTINATION include/megastructure )
install( TARGETS megastructurelib DESTINATION lib)