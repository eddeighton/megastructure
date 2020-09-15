
cmake_minimum_required( VERSION 3.1...3.16 )

#get boost
include( ${MEGA_ROOT_DIR}/cmake/boost_include.cmake )

#get xsde
include( ${MEGA_ROOT_DIR}/cmake/xsde_include.cmake )

#get pybind11
include( ${MEGA_ROOT_DIR}/cmake/pybind11_include.cmake )

#get common
include( ${MEGA_ROOT_DIR}/cmake/common_include.cmake )

#eg
include( ${MEGA_ROOT_DIR}/cmake/eg_include.cmake )

set( MEGA_PYTHON_HOST_DIR ${MEGA_ROOT_DIR}/src/hosts/python )
#get the sources

set( MEGA_PYTHON_HOST_SOURCES
	${MEGA_PYTHON_HOST_DIR}/python_host.cpp
    )
	
#pybind11_add_module( python_host ${MEGA_PYTHON_HOST_SOURCES} )
    
add_library( python_host MODULE ${MEGA_PYTHON_HOST_SOURCES} )
		
set_target_properties( python_host PROPERTIES FOLDER python )
set_target_properties( python_host PROPERTIES DEBUG_POSTFIX d )

link_boost( python_host program_options )
link_boost( python_host filesystem )
link_boost( python_host system )
link_boost( python_host date_time )
link_xsde( python_host )
link_pybind11_as_module( python_host )
link_common( python_host )
link_eg_runtime( python_host )

target_link_libraries( python_host megastructurelib )
target_link_libraries( python_host megastructureprotocollib )
target_link_libraries( python_host componentlib )

install( TARGETS python_host DESTINATION bin)
