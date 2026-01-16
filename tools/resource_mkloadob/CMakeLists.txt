add_executable( cdc_lib_resource_mkloadob )
target_sources( cdc_lib_resource_mkloadob PRIVATE resource_mkloadob.cpp )
target_link_libraries( cdc_lib_resource_mkloadob PUBLIC cdc_lib cxxopts fmt::fmt )
