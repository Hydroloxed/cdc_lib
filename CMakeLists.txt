cmake_minimum_required( VERSION 3.10 )

project( cdc_lib )

option( CDC_LIB_TARGET_TRAS "Support TRAS (2013)" ON )
option( CDC_LIB_BUILD_TOOLS "Build Tools" ON )

add_library( cdc_lib INTERFACE )

add_subdirectory( src/cdc_lib/core )
add_subdirectory( src/cdc_lib/file )
add_subdirectory( src/cdc_lib/render )
add_subdirectory( src/cdc_lib/resource )

add_subdirectory( tests )

target_link_libraries( cdc_lib INTERFACE cdc_lib_core cdc_lib_file cdc_lib_render cdc_lib_resource )

target_include_directories( cdc_lib INTERFACE
  src )

if( CDC_LIB_BUILD_TOOLS )
    add_subdirectory( tools )
endif()
