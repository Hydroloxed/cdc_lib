
add_executable( cdc_lib_resolve_object_dump )

target_sources( cdc_lib_resolve_object_dump PRIVATE resolve_object_dump.cpp )

target_link_libraries( cdc_lib_resolve_object_dump PUBLIC cdc_lib cxxopts fmt::fmt )
