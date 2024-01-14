add_executable( cdc_lib_section_relocation_dump )
target_sources( cdc_lib_section_relocation_dump PRIVATE section_relocation_dump.cpp )
target_link_libraries( cdc_lib_section_relocation_dump PUBLIC cdc_lib cxxopts fmt::fmt )