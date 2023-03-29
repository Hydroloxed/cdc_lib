add_library( cdc_lib_resource )

if( CMAKE_CXX_COMPILER_ID MATCHES "Clang" )
    target_compile_options( cdc_lib_resource PRIVATE -Wall -Werror -pedantic-errors -Wextra )
elseif( CMAKE_CXX_COMPILER_ID MATCHES "GNU" )
    target_compile_options( cdc_lib_resource PRIVATE -Wall -Werror -pedantic-errors -Wextra )
endif()

if( CDC_LIB_TARGET_TRAS )
    target_sources( cdc_lib_resource PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/tras/pc_w/tras_pc_w_relocation.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tras/pc_w/tras_pc_w_relocation.h
        ${CMAKE_CURRENT_SOURCE_DIR}/tras/pc_w/tras_pc_w_resource.h
        ${CMAKE_CURRENT_SOURCE_DIR}/rsrc_reloc_input_stream.h
        ${CMAKE_CURRENT_SOURCE_DIR}/rsrc_relocation.h
        ${CMAKE_CURRENT_SOURCE_DIR}/rsrc_resource.h )
endif()
target_compile_features( cdc_lib_resource PUBLIC cxx_std_20 )
target_include_directories( cdc_lib_resource PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../ )
target_link_libraries( cdc_lib_resource PRIVATE cdc_lib_core )
