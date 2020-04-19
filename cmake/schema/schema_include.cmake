

cmake_minimum_required(VERSION 3.14.2)

function( link_schemalib targetname )
	target_include_directories( ${targetname} PRIVATE ${MEGA_ROOT_DIR}/src/schema )
	target_link_libraries( ${targetname} schemalib )
endfunction( link_schemalib )
