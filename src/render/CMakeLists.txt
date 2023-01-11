add_library( cdc_lib_render )

if( CMAKE_CXX_COMPILER_ID MATCHES "Clang" )
    target_compile_options( cdc_lib_render PRIVATE -Wall -Werror -pedantic-errors -Wextra )
elseif( CMAKE_CXX_COMPILER_ID MATCHES "GNU" )
    target_compile_options( cdc_lib_render PRIVATE -Wall -Werror -pedantic-errors -Wextra )
endif()

if( CDC_LIB_TARGET_TRAS )
    target_sources( cdc_lib_render PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/pc_w/tras/pc_w_tras_shader.h
        ${CMAKE_CURRENT_SOURCE_DIR}/pc_w/tras/pc_w_tras_shader.cpp )
endif()
target_compile_features( cdc_lib_render PUBLIC cxx_std_20 )
target_include_directories( cdc_lib_render PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../ )
target_link_libraries( cdc_lib_render PRIVATE cdc_lib_core )
