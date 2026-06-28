add_executable( cdc_lib_tests )

include( FetchContent )

FetchContent_Declare( ut
  GIT_REPOSITORY https://github.com/boost-ext/ut.git )
FetchContent_MakeAvailable( ut )

target_sources( cdc_lib_tests PRIVATE
    core/test_crc32.cpp
    file/test_compression.cpp
    resource/test_reloc_input_stream.cpp
    sdef/test_sdef.cpp
    run_tests.cpp )
target_link_libraries( cdc_lib_tests PUBLIC cdc_lib Boost::ut )
