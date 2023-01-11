add_library( cdc_lib_core INTERFACE )

target_link_libraries( cdc_lib_core INTERFACE score )
target_compile_features( cdc_lib_core INTERFACE cxx_std_20 )
