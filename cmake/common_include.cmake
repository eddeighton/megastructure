cmake_minimum_required(VERSION 2.8)

find_path( COMMON_INSTALL_PATH /share/common-config.cmake PATHS ${MEGA_ROOT_DIR}/../../common/install REQUIRED )

#message( "COMMON_INSTALL_PATH: ${COMMON_INSTALL_PATH}" )

#set( CMAKE_PREFIX_PATH "${COMMON_INSTALL_PATH}/share;${CMAKE_PREFIX_PATH}" )
set( common_DIR ${COMMON_INSTALL_PATH}/share ) 

find_package( common )

function( link_common targetname )
	target_link_libraries( ${targetname} Common::commonlib )
endfunction( link_common )
