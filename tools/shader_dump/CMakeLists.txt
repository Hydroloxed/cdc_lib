
add_executable( cdc_lib_shader_dump )

target_sources( cdc_lib_shader_dump PRIVATE shader_dump.cpp )

target_link_libraries( cdc_lib_shader_dump PUBLIC cdc_lib cxxopts fmt::fmt )
