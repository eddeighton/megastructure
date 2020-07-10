
cmake_minimum_required( VERSION 3.1...3.16 )

#get boost
include( ${MEGA_ROOT_DIR}/cmake/boost_include.cmake )

#get clang
include( ${MEGA_ROOT_DIR}/cmake/clang_include.cmake )

#get xsde
include( ${MEGA_ROOT_DIR}/cmake/xsde_include.cmake )

#get pybind11
include( ${MEGA_ROOT_DIR}/cmake/pybind11_include.cmake )

#get common
include( ${MEGA_ROOT_DIR}/cmake/common_include.cmake )

#eg
include( ${MEGA_ROOT_DIR}/cmake/eg_include.cmake )

set( MEGA_PYTHON_HOST_DIR ${MEGA_ROOT_DIR}/src/python )
#get the sources

set( MEGA_PYTHON_LIB_HEADERS
    ${MEGA_PYTHON_HOST_DIR}/python_iterator.hpp
    ${MEGA_PYTHON_HOST_DIR}/python_reference.hpp
    ${MEGA_PYTHON_HOST_DIR}/python_reference_factory.hpp
    )
    
set( MEGA_PYTHON_LIB_SOURCES
    ${MEGA_PYTHON_HOST_DIR}/python_iterator.cpp
    ${MEGA_PYTHON_HOST_DIR}/python_reference.cpp
    ${MEGA_PYTHON_HOST_DIR}/python_reference_factory.cpp
    )
    
add_library( python_lib ${MEGA_PYTHON_LIB_HEADERS} ${MEGA_PYTHON_LIB_SOURCES} )
		
set_target_properties( python_lib PROPERTIES FOLDER python )
set_target_properties( python_lib PROPERTIES DEBUG_POSTFIX d )

link_boost( python_lib program_options )
link_boost( python_lib filesystem )
link_boost( python_lib system )
link_boost( python_lib date_time )
link_xsde( python_lib )
link_clang( python_lib )
link_pybind11( python_lib )
link_common( python_lib )
link_eg( python_lib )

#target_link_libraries( python_lib megastructurelib )
#target_link_libraries( python_lib megastructureprotocollib )
#target_link_libraries( python_lib componentlib )

install( FILES ${MEGA_PYTHON_LIB_HEADERS} DESTINATION include/python_lib )
install( TARGETS python_lib EXPORT mega DESTINATION lib)
install( EXPORT mega NAMESPACE Mega:: FILE mega-config.cmake DESTINATION share )