add_executable( cdc_lib_resource_make_loose )
target_sources( cdc_lib_resource_make_loose PRIVATE resource_make_loose.cpp )
target_link_libraries( cdc_lib_resource_make_loose PUBLIC cdc_lib cxxopts fmt::fmt )
