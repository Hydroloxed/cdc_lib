add_library( cdc_lib_file )

if( CMAKE_CXX_COMPILER_ID MATCHES "Clang" )
    target_compile_options( cdc_lib_file PRIVATE -Wall -Werror -pedantic-errors -Wextra )
elseif( CMAKE_CXX_COMPILER_ID MATCHES "GNU" )
    target_compile_options( cdc_lib_file PRIVATE -Wall -Werror -pedantic-errors -Wextra )
endif()

if( CDC_LIB_TARGET_TRAS )
    target_sources( cdc_lib_file PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/tras/pc_w/tras_pc_w_compression.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tras/pc_w/tras_pc_w_compression.h )
endif()

target_sources( cdc_lib_file PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/archive_tiger.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/archive_fs.h )

find_package( ZLIB REQUIRED )

target_compile_features( cdc_lib_file PUBLIC cxx_std_20 )
target_include_directories( cdc_lib_file PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../ )
target_link_libraries( cdc_lib_file PRIVATE cdc_lib_core fmt ZLIB::ZLIB )
target_compile_definitions( cdc_lib_file PRIVATE ZLIB_CONST )
