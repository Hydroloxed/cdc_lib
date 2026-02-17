add_library( cdc_lib_sdef )

if( CMAKE_CXX_COMPILER_ID MATCHES "Clang" )
   target_compile_options( cdc_lib_sdef PRIVATE -Wall -Werror -pedantic-errors -Wextra )
elseif( CMAKE_CXX_COMPILER_ID MATCHES "GNU" )
   target_compile_options( cdc_lib_sdef PRIVATE -Wall -Werror -pedantic-errors -Wextra )
endif()

target_sources( cdc_lib_sdef PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/sdef_node.h
    ${CMAKE_CURRENT_SOURCE_DIR}/sdef_parse.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sdef_parse.h
    ${CMAKE_CURRENT_SOURCE_DIR}/sdef_type.h )

include( FetchContent )
FetchContent_Declare( tinyxml2
      GIT_REPOSITORY https://github.com/leethomason/tinyxml2.git )
FetchContent_MakeAvailable( tinyxml2 )

target_compile_features( cdc_lib_sdef PUBLIC cxx_std_20 )
target_include_directories( cdc_lib_sdef PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../ )
target_link_libraries( cdc_lib_sdef PRIVATE cdc_lib_core fmt tinyxml2 )
