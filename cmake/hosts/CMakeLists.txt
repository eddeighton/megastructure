
cmake_minimum_required( VERSION 3.1...3.16 )

add_subdirectory(basic)
add_subdirectory(geometry)
add_subdirectory(hostdll)
add_subdirectory(python)

#include eg files
install( FILES ${MEGA_API_DIR}/hosts/basic/.eg DESTINATION include/hosts/basic )
install( FILES ${MEGA_API_DIR}/hosts/geometry/.eg DESTINATION include/hosts/geometry )
install( FILES ${MEGA_API_DIR}/hosts/python/.eg DESTINATION include/hosts/python )
install( FILES ${MEGA_API_DIR}/hosts/unreal/.eg DESTINATION include/hosts/unreal )
