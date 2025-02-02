add_executable( cdc_lib_resource_db )
target_sources( cdc_lib_resource_db PRIVATE resource_db_tool.cpp )
target_link_libraries( cdc_lib_resource_db PUBLIC cdc_lib cxxopts fmt::fmt )
