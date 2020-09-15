
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

set( MEGA_UNREAL_HOST_DIR ${MEGA_ROOT_DIR}/src/unreal )

##get the sources
set( MEGA_UNREAL_LIB_HEADERS
    ${MEGA_UNREAL_HOST_DIR}/unreal.hpp
    )

install( FILES ${MEGA_UNREAL_LIB_HEADERS} DESTINATION include/unreal )
#install( TARGETS python_lib EXPORT mega DESTINATION lib)
#install( EXPORT mega NAMESPACE Mega:: FILE mega-config.cmake DESTINATION share )