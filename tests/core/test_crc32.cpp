#include <boost/ut.hpp>
#include <cdc_lib/core/core_crc32.h>

using namespace boost::ut;
using namespace cdc_lib::core;

suite< "core_crc32" > core_crc32 = []
{
    /* See https://reveng.sourceforge.io/crc-catalogue/17plus.htm#crc.cat-bits.32
     * For info about different crc algorithms. We use CRC-32/BZIP,
     * with width=32 poly=0x04c11db7 init=0xffffffff.
     */
    test( "empty data" ) = []
    {
        expect( crc32( "" ) == 0_u32 );
        expect( crc32( std::span< std::byte >{} ) == 0_u32 );
    };

    test( "123456789" ) = []
    {
        expect( crc32( "123456789") == 0xfc891918 );
    };

    test( "same result when we use bytes and strings" ) = []
    {
        const std::string_view sample_string = "123456789";
        const std::span< const std::byte > sample_bytes =
            std::as_bytes( std::span{sample_string.data(), sample_string.size()} );
        expect( crc32( sample_string ) == crc32( sample_bytes ) );
    };

    test( "same as what cdc uses" ) = []
    {
        expect( crc32( "pc-w\\fishing_rig.drm" ) == 0xb46bc2fe );
    };
};