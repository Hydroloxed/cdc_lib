add_executable( cdc_lib_archive_dump )
target_sources( cdc_lib_archive_dump PRIVATE archive_dump.cpp )
target_link_libraries( cdc_lib_archive_dump PUBLIC cdc_lib cxxopts fmt::fmt )
