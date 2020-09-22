

cmake_minimum_required( VERSION 3.1...3.16 )

find_path( UNREAL_SDK Engine/Source/UE4Editor.Target.cs PATHS "X:/UE_4.25" REQUIRED )
find_path( UNREAL_LIB_PATH Core/UE4Editor-Core.lib PATHS "${UNREAL_SDK}/Engine/Intermediate/Build/Win64/UE4Editor/Development" REQUIRED )

function( link_unreal_math targetname )

    target_include_directories( ${targetname} PRIVATE "${UNREAL_SDK}/Engine/Source"                          )
    target_include_directories( ${targetname} PRIVATE "${UNREAL_SDK}/Engine/Source/Runtime"                  )
    target_include_directories( ${targetname} PRIVATE "${UNREAL_SDK}/Engine/Source/Runtime/Core/Public"      )
    target_include_directories( ${targetname} PRIVATE "${UNREAL_SDK}/Engine/Source/Runtime/TraceLog/Public"  )
    
    target_link_libraries( ${targetname} ${UNREAL_LIB_PATH}/Core/UE4Editor-Core.lib )
endfunction( link_unreal_math )
