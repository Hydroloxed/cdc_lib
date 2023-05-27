
add_executable( cdc_lib_material_dump )

target_sources( cdc_lib_material_dump PRIVATE material_dump.cpp )

target_link_libraries( cdc_lib_material_dump PUBLIC cdc_lib )
