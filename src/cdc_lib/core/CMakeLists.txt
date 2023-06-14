add_library( cdc_lib_core INTERFACE )

include( FetchContent )

FetchContent_Declare( cxxopts
  GIT_REPOSITORY https://github.com/jarro2783/cxxopts.git )
FetchContent_MakeAvailable( cxxopts )

FetchContent_Declare( fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG 10.0.0 )
FetchContent_MakeAvailable( fmt )

target_link_libraries( cdc_lib_core INTERFACE score )
target_compile_features( cdc_lib_core INTERFACE cxx_std_20 )
