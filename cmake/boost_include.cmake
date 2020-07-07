
cmake_minimum_required( VERSION 3.1...3.16 )

include( ${MEGA_THIRD_PARTY_DIR}/boost/install/lib/cmake/Boost-1.73.0/BoostConfig.cmake )

######################################
#Sort out the boost dependencies
find_path( BOOST_INCLUDEDIR NAMES boost PATHS ${MEGA_THIRD_PARTY_DIR}/boost/install/include/boost-1_73 )
find_path( BOOST_LIBRARYDIR NAMES "cmake/Boost-1.73.0/BoostConfig.cmake" PATHS ${MEGA_THIRD_PARTY_DIR}/boost/install/lib )

set(BOOST_REQUIRED_VERSION 1.73.0 )
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package( Boost 1.73.0 REQUIRED QUIET COMPONENTS 
    program_options
    date_time
    random
    filesystem
    system
    iostreams
    serialization
	timer )

function( compiler_define_boost targetname )
	target_compile_definitions( ${targetname} PRIVATE -DBOOST_ENABLE_ASSERT_HANDLER -D_BOOST_ENABLE_ASSERT_HANDLER )
	target_compile_definitions( ${targetname} PRIVATE -DBOOST_CONFIG_SUPPRESS_OUTDATED_MESSAGE -D_BOOST_CONFIG_SUPPRESS_OUTDATED_MESSAGE )
endfunction( compiler_define_boost )

function( link_boost targetname lib )
	compiler_define_boost( ${targetname} )
    target_link_libraries( ${targetname} Boost::${lib} )
endfunction( link_boost )

function( link_boost_usual targetname )
	compiler_define_boost( ${targetname} )
    target_link_libraries( ${targetname} Boost::program_options )
    target_link_libraries( ${targetname} Boost::filesystem )
    target_link_libraries( ${targetname} Boost::system )
endfunction( link_boost_usual )
