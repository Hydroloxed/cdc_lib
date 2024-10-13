add_executable( cdc_lib_resource_refdata )
target_sources( cdc_lib_resource_refdata PRIVATE resource_refdata.cpp )
target_link_libraries( cdc_lib_resource_refdata PUBLIC cdc_lib cxxopts fmt::fmt )