add_library( cdc_lib_core INTERFACE )

include( FetchContent )

FetchContent_Declare( cxxopts
  GIT_REPOSITORY https://github.com/jarro2783/cxxopts.git )
FetchContent_MakeAvailable( cxxopts )

FetchContent_Declare( fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG 10.0.0 )
FetchContent_MakeAvailable( fmt )

FetchContent_Declare( score 
  GIT_REPOSITORY https://gitlab.com/Hydrolox/score.git )
FetchContent_MakeAvailable( score )

target_sources( cdc_lib_core INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/core_crc32.h )

target_link_libraries( cdc_lib_core INTERFACE score )
target_compile_features( cdc_lib_core INTERFACE cxx_std_20 )
