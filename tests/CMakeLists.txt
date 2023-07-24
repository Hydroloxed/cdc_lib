add_executable( cdc_lib_tests )
target_sources( cdc_lib_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/test_reloc_input_stream.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/run_tests.cpp )
target_link_libraries( cdc_lib_tests PUBLIC cdc_lib Boost::ut )
