add_executable( cdc_lib_tests )

include( FetchContent )

FetchContent_Declare( Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.16.1 )
FetchContent_MakeAvailable( Catch2 )

target_sources( cdc_lib_tests PRIVATE
    core/test_crc32.cpp
    file/test_compression.cpp
    resource/test_reloc_input_stream.cpp
    resource/test_resolve_object.cpp
    sdef/test_sdef.cpp
    sdef/test_sobj.cpp )
target_link_libraries( cdc_lib_tests PUBLIC cdc_lib Catch2::Catch2WithMain )
