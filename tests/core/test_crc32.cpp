#include <catch2/catch_test_macros.hpp>
#include <cdc_lib/core/core_crc32.h>

using namespace cdc_lib::core;

TEST_CASE( "crc32", "[core]" )
{
    /* See https://reveng.sourceforge.io/crc-catalogue/17plus.htm#crc.cat-bits.32
     * For info about different crc algorithms. We use CRC-32/BZIP,
     * with width=32 poly=0x04c11db7 init=0xffffffff.
     */
    SECTION( "empty data" )
    {
        CHECK( crc32( "" ) == 0 );
        CHECK( crc32( std::span< std::byte >{} ) == 0 );
    };

    SECTION( "123456789" )
    {
        CHECK( crc32( "123456789") == 0xfc891918 );
    };

    SECTION( "same result when we use bytes and strings" )
    {
        const std::string_view sample_string = "123456789";
        const std::span< const std::byte > sample_bytes =
            std::as_bytes( std::span{sample_string.data(), sample_string.size()} );
        CHECK( crc32( sample_string ) == crc32( sample_bytes ) );
    };

    SECTION( "same as what cdc uses" )
    {
        CHECK( crc32( "pc-w\\fishing_rig.drm" ) == 0xb46bc2fe );
    };
};