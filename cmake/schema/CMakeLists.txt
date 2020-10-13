
cmake_minimum_required( VERSION 3.1...3.16 )

#get xsde
include( ${MEGA_ROOT_DIR}/cmake/xsde_include.cmake )
    
#get boost
include( ${MEGA_ROOT_DIR}/cmake/boost_include.cmake )

#get common
include( ${MEGA_ROOT_DIR}/cmake/common_include.cmake )

#handle the schema code gen
set( SCHEMA_SOURCE_DIR ${MEGA_ROOT_DIR}/src/schema )

find_file( MEGA_SCHEMA_FILE NAME mega_schema.xsd PATHS ${SCHEMA_SOURCE_DIR} )
set( MEGA_SCHEMA_GENERATED_SOURCE_DIR ${SCHEMA_SOURCE_DIR}/megaxml )

set( MEGA_SCHEMA_GENERATED_HEADERS
        ${MEGA_SCHEMA_GENERATED_SOURCE_DIR}/mega_schema.hxx
        ${MEGA_SCHEMA_GENERATED_SOURCE_DIR}/mega_schema-pskel.hxx
        ${MEGA_SCHEMA_GENERATED_SOURCE_DIR}/mega_schema-pimpl.hxx
        ${MEGA_SCHEMA_GENERATED_SOURCE_DIR}/mega_schema-sskel.hxx
        ${MEGA_SCHEMA_GENERATED_SOURCE_DIR}/mega_schema-simpl.hxx )
set( MEGA_SCHEMA_GENERATED_SOURCE
        ${MEGA_SCHEMA_GENERATED_SOURCE_DIR}/mega_schema.cxx
        ${MEGA_SCHEMA_GENERATED_SOURCE_DIR}/mega_schema-pskel.cxx
        ${MEGA_SCHEMA_GENERATED_SOURCE_DIR}/mega_schema-pimpl.cxx
        ${MEGA_SCHEMA_GENERATED_SOURCE_DIR}/mega_schema-sskel.cxx
        ${MEGA_SCHEMA_GENERATED_SOURCE_DIR}/mega_schema-simpl.cxx )

set( MEGA_SCHEMA_GENERATED_FILES ${MEGA_SCHEMA_GENERATED_HEADERS} ${MEGA_SCHEMA_GENERATED_SOURCE} )

#compile_schema( mega_schema_target ${MEGA_SCHEMA_FILE} "megaxml" ${MEGA_SCHEMA_GENERATED_SOURCE_DIR} ${MEGA_SCHEMA_GENERATED_FILES} )
	
add_custom_command( COMMAND ${XSDE_EXECUTABLE} 
	ARGS "cxx-hybrid" --generate-parser --generate-serializer --generate-aggregate --no-long-long --namespace-map ="megaxml" --output-dir ${MEGA_SCHEMA_GENERATED_SOURCE_DIR} ${MEGA_SCHEMA_FILE}
	MAIN_DEPENDENCY ${MEGA_SCHEMA_FILE}
	DEPENDS ${MEGA_SCHEMA_FILE}
	OUTPUT ${MEGA_SCHEMA_GENERATED_FILES}
	COMMENT "Generating xml schema parser using xsde for ${MEGA_SCHEMA_FILE} to ${MEGA_SCHEMA_GENERATED_SOURCE_DIR}"
)
   
set( OTHER_HEADERS
        ${SCHEMA_SOURCE_DIR}/environment_string.hpp
        ${SCHEMA_SOURCE_DIR}/project.hpp
        ${SCHEMA_SOURCE_DIR}/projectTree.hpp )
set( OTHER_SOURCES
        ${SCHEMA_SOURCE_DIR}/environment_string.cpp
        ${SCHEMA_SOURCE_DIR}/project.cpp
        ${SCHEMA_SOURCE_DIR}/projectTree.cpp )
        
add_library( schemalib 
                    ${MEGA_SCHEMA_GENERATED_HEADERS} 
					${MEGA_SCHEMA_GENERATED_SOURCE}
                    ${OTHER_HEADERS} ${OTHER_SOURCES}
				)

#add_dependencies( schemalib mega_schema_target )

set_target_properties( schemalib PROPERTIES FOLDER driver )
set_target_properties( schemalib PROPERTIES DEBUG_POSTFIX d )

link_xsde( schemalib )
link_boost( schemalib filesystem )
link_common( schemalib )

install( FILES  ${OTHER_HEADERS} DESTINATION include/schema )
install( FILES  ${MEGA_SCHEMA_GENERATED_HEADERS} DESTINATION include/schema/megaxml )
install( TARGETS schemalib EXPORT mega DESTINATION lib )
install( EXPORT mega NAMESPACE Mega:: FILE mega-config.cmake DESTINATION share )
install( FILES  ${MEGA_ROOT_DIR}/src/schema/.eg DESTINATION include/schema )
                
                
