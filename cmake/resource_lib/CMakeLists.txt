
cmake_minimum_required( VERSION 3.1...3.16 )

#get boost
include( ${MEGA_ROOT_DIR}/cmake/boost_include.cmake )

#get xsde
include( ${MEGA_ROOT_DIR}/cmake/xsde_include.cmake )

#get spdlog
#include( ${MEGA_ROOT_DIR}/cmake/spdlog_include.cmake )

#get mega schema
include( ${MEGA_ROOT_DIR}/cmake/schema/schema_include.cmake )

#get common
include( ${MEGA_ROOT_DIR}/cmake/common_include.cmake )

#get ed
include( ${MEGA_ROOT_DIR}/cmake/ed_include.cmake )

#get blueprint
include( ${MEGA_ROOT_DIR}/cmake/blueprint_include.cmake )

set( MEGA_RESOURCE_LIB_HEADERS
    ${MEGA_SRC_DIR}/resid/resource.hpp
    ${MEGA_SRC_DIR}/resid/blueprint.hpp
    )
    
set( MEGA_RESOURCE_LIB_SOURCES
    ${MEGA_SRC_DIR}/resid/resource.cpp
    ${MEGA_SRC_DIR}/resid/blueprint.cpp
    )
    
add_library( resource_lib ${MEGA_RESOURCE_LIB_HEADERS} ${MEGA_RESOURCE_LIB_SOURCES} )
		
set_target_properties( resource_lib PROPERTIES FOLDER resource )
set_target_properties( resource_lib PROPERTIES DEBUG_POSTFIX d )

link_boost( resource_lib filesystem )
link_boost( resource_lib system )
link_boost( resource_lib date_time )
link_xsde( resource_lib )
link_schemalib( resource_lib )
link_common( resource_lib )
link_ed( resource_lib )
link_blueprint( resource_lib )

install( FILES ${MEGA_RESOURCE_LIB_HEADERS} DESTINATION include/resource_lib )
install( TARGETS resource_lib EXPORT mega DESTINATION lib)
install( EXPORT mega NAMESPACE Mega:: FILE mega-config.cmake DESTINATION share )