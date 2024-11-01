add_executable( cdc_lib_file_decompress )
target_sources( cdc_lib_file_decompress PRIVATE file_decompress.cpp )
target_link_libraries( cdc_lib_file_decompress PUBLIC cdc_lib cxxopts fmt::fmt )
