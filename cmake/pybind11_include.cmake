
cmake_minimum_required( VERSION 3.1...3.16 )

include( ${MEGA_THIRD_PARTY_DIR}/pybind11/install/share/cmake/pybind11/pybind11Config.cmake )

find_path( pybind11_DIR NAMES pybind11Config.cmake PATHS ${MEGA_THIRD_PARTY_DIR}/pybind11/install/share/cmake/pybind11 )

find_package( pybind11 REQUIRED)

function( link_pybind11 targetname )
	target_compile_definitions( ${targetname} PRIVATE -DHAVE_SNPRINTF ) #python will do #define snprintf _snprintf if not HAVE_SNPRINTF
    target_link_libraries( ${targetname} pybind11::module ) 
endfunction( link_pybind11 )

function( link_pybind11_as_module targetname )
    link_pybind11( ${targetname} )
    set_target_properties( ${targetname} PROPERTIES 
                                         SUFFIX "${PYTHON_MODULE_EXTENSION}" )
endfunction( link_pybind11 )

