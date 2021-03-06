
cmake_minimum_required( VERSION 3.1...3.16 )

#protobuf
include( ${MEGA_ROOT_DIR}/cmake/protobuf_include.cmake )

#folders
set( MEGASTRUCTURE_PROTOCOL_API_DIR ${MEGA_API_DIR}/protocol )
set( MEGASTRUCTURE_PROTOCOL_SRC_DIR ${MEGA_SRC_DIR}/protocol )
set( MEGASTRUCTURE_PROTOCOL_SRC_GEN_DIR ${MEGA_SRC_DIR}/protocol/generated )

#files
set( MEGASTRUCTURE_PROTOCOL ${MEGASTRUCTURE_PROTOCOL_SRC_DIR}/megastructure.proto )
set( MEGASTRUCTURE_PROTOCOL_HEADER ${MEGASTRUCTURE_PROTOCOL_SRC_GEN_DIR}/megastructure.pb.h )
set( MEGASTRUCTURE_PROTOCOL_SOURCE ${MEGASTRUCTURE_PROTOCOL_SRC_GEN_DIR}/megastructure.pb.cc )
set( MEGASTRUCTURE_PROTOCOL_API 
	${MEGASTRUCTURE_PROTOCOL_API_DIR}/megastructure.pb.h
	${MEGASTRUCTURE_PROTOCOL_API_DIR}/protocol_helpers.hpp )

add_custom_command(
	  PRE_BUILD
      OUTPUT "${MEGASTRUCTURE_PROTOCOL_SOURCE}"
             "${MEGASTRUCTURE_PROTOCOL_HEADER}"
      COMMAND  ${PROTOBUF_PROTOC_EXECUTABLE}
      ARGS --cpp_out=${MEGASTRUCTURE_PROTOCOL_SRC_GEN_DIR} --proto_path=${MEGASTRUCTURE_PROTOCOL_SRC_DIR} ${MEGASTRUCTURE_PROTOCOL}
      DEPENDS ${MEGASTRUCTURE_PROTOCOL}
      COMMENT "Running C++ protocol buffer compiler for the mega structure protocol"
      VERBATIM )

add_custom_command(
    OUTPUT ${MEGASTRUCTURE_PROTOCOL_API_DIR}/megastructure.pb.h
    COMMAND ${CMAKE_COMMAND} -E copy ${MEGASTRUCTURE_PROTOCOL_HEADER} ${MEGASTRUCTURE_PROTOCOL_API_DIR}/megastructure.pb.h
    MAIN_DEPENDENCY ${MEGASTRUCTURE_PROTOCOL_HEADER} )

add_library( megastructureprotocollib 
	${MEGASTRUCTURE_PROTOCOL_API} 
	${MEGASTRUCTURE_PROTOCOL_SOURCE} )
	
set_target_properties( megastructureprotocollib PROPERTIES FOLDER core )
set_target_properties( megastructureprotocollib PROPERTIES DEBUG_POSTFIX d )

link_protobuf( megastructureprotocollib )

install( FILES ${MEGASTRUCTURE_PROTOCOL_API} DESTINATION include/megastructureprotocol )
install( TARGETS megastructureprotocollib EXPORT mega DESTINATION lib )
install( EXPORT mega NAMESPACE Mega:: FILE mega-config.cmake DESTINATION share )
