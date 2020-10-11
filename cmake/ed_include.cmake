
cmake_minimum_required( VERSION 3.1...3.16 )

find_path( ED_INSTALL_PATH share/ed-config.cmake PATHS ${MEGA_ROOT_DIR}/../../ed/install )

include( ${ED_INSTALL_PATH}/share/ed-config.cmake )

function( link_ed targetname )
	target_include_directories( ${targetname} PRIVATE ${ED_INSTALL_PATH}/include )
	target_link_libraries( ${targetname} ED::edlib )
endfunction( link_ed )
