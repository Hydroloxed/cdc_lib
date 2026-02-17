add_executable( cdc_lib_sdef_compile )
target_sources( cdc_lib_sdef_compile PRIVATE sdef_compile.cpp )
target_link_libraries( cdc_lib_sdef_compile PUBLIC cdc_lib cxxopts fmt::fmt )
