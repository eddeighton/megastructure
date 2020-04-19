

cmake_minimum_required(VERSION 3.14.2)

find_path( XSDE_BUILD_DEBUG_DIR NAMES xsde/xsde.lib PATHS ${MEGA_THIRD_PARTY_DIR}/xsde/install/debug )
find_path( XSDE_BUILD_RELEASE_DIR NAMES xsde/xsde.lib PATHS ${MEGA_THIRD_PARTY_DIR}/xsde/install/release )

find_file( XSDE_DEBUG_LIB NAME xsde.lib PATHS ${XSDE_BUILD_DEBUG_DIR}/xsde )
find_file( XSDE_RELEASE_LIB NAME xsde.lib PATHS ${XSDE_BUILD_RELEASE_DIR}/xsde )

find_file( XSDE_EXECUTABLE NAME xsde.exe PATHS ${MEGA_THIRD_PARTY_DIR}/xsde/src/xsde-3.2.0-i686-windows/bin )

#INCLUDE_DIRECTORIES( optimized ${XSDE_BUILD_RELEASE_DIR} debug ${XSDE_BUILD_DEBUG_DIR} )
#LINK_DIRECTORIES( optimized ${XSDE_BUILD_RELEASE_DIR} debug ${XSDE_BUILD_DEBUG_DIR} )

function( link_xsde targetname )
	target_include_directories( ${targetname} PRIVATE optimized ${XSDE_BUILD_RELEASE_DIR} debug ${XSDE_BUILD_DEBUG_DIR} )
	target_link_directories( ${targetname} PRIVATE optimized ${XSDE_BUILD_RELEASE_DIR} debug ${XSDE_BUILD_DEBUG_DIR} )
    target_link_libraries( ${targetname} optimized ${XSDE_RELEASE_LIB} debug ${XSDE_DEBUG_LIB} )
endfunction( link_xsde )

#macro( compile_schema xml_schema nmspace output_directory output_files )
#    add_custom_command( COMMAND ${XSDE_EXECUTABLE} 
#        ARGS "cxx-hybrid" --generate-parser --generate-serializer --generate-aggregate --no-long-long --namespace-map =${nmspace} --output-dir ${output_directory} ${xml_schema}
#        MAIN_DEPENDENCY ${xml_schema}
#		DEPENDS ${xml_schema}
#        OUTPUT ${output_files}
#        COMMENT "Generating xml schema parser using xsde for ${xml_schema} to ${output_directory}"
#    )
#	set_source_files_properties( ${output_files} GENERATED )
#endmacro( compile_schema )