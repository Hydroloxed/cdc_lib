#include "score/binary_io/binary_io.h"
#include <boost/ut.hpp>
#include <cdc_lib/file/tras/pc_w/tras_pc_w_compression.h>

using namespace boost::ut;
using namespace cdc_lib::file::tras::pc_w;

namespace
{
    auto decompress(const char* a_data, std::size_t a_size)
    {
        std::string_view data_str{a_data, a_size};
        auto ii = score::binary_io::create_input_interface(data_str);
        return decompress_cdrm( *ii );
    }
}

suite< "file_compression" > file_compression = []
{
    test( "no blocks" ) = []
    {
        const char data[] =
            "CDRM" // magic
            "\0\0\0\0" // version
            "\0\0\0\0" // block count
            "\0\0\0\0" // num padding bytes
            ;
        expect( eq( decompress( data, sizeof data ), std::string{} ) );
    };

    test( "exception thrown if magic is not CDRM" ) = []
    {
        const char data[] = "NOPE";
        expect( throws( [&]{ decompress( data, sizeof data ); } ) );
    };

    // FIXME: Reactivate when padding is properly tested and implemented
    //        (Only if we can find it used by the game)
    skip / test( "exception thrown if padding count is nonzero" ) = []
    {
        const char data[] =
            "CDRM" // magic
            "\0\0\0\0" // version
            "\0\0\0\0" // block count
            "\1\0\0\0" // num padding bytes
            "\0"; // padding byte
        expect( throws( [&]{ decompress( data, sizeof data ); } ) );
    };

    test( "exception thrown if NEXT marker is invalid" ) = []
    {
        const char data[] =
            "CDRM" // magic
            "\0\0\0\0" // version
            "\0\0\0\0" // block count
            "\0\0\0\0" // num padding bytes
            "JUNK"; // invalid NEXT marker
        expect( throws( [&]{ decompress( data, sizeof data ); } ) );
    };
    
    test( "exception thrown if wrong version" ) = []
    {
        const char data[] =
            "CDRM" // magic
            "\1\2\3\4" // version
            "\0\0\0\0" // block count
            "\0\0\0\0"; // num padding bytes
        expect( throws( [&]{ decompress( data, sizeof data ); } ) );
    };

    test( "exception thrown if uncompressed size is too large" ) = []
    {
        const char data[] =
            "CDRM" // magic
            "\0\0\0\0" // version
            "\1\0\0\0" // block count
            "\0\0\0\0" // num padding bytes
            "\1" // type=uncompressed
            "\xff\xff\xff" // uncompressed size
            "\0\0\0\0"; // compressed size
        expect( throws( [&]{ decompress( data, sizeof data ); } ) );
    };

    test( "exception thrown if compressed size is too large" ) = []
    {
        const char data[] =
            "CDRM" // magic
            "\0\0\0\0" // version
            "\1\0\0\0" // block count
            "\0\0\0\0" // num padding bytes
            "\1" // type=uncompressed
            "\1\0\0" // uncompressed size
            "\xff\xff\xff\xff"; // compressed size
        expect( throws( [&]{ decompress( data, sizeof data ); } ) );
    };

    // FIXME: Disabled for now, change compression.cpp to accept empty
    // blocks, and then reenable this test.
    skip / test( "empty block ignored" ) = []
    {
        const char data[] =
            "CDRM" // magic
            "\0\0\0\0" // version
            "\1\0\0\0" // block count
            "\0\0\0\0" // num padding bytes
            // block 0
            "\0" // type=empty
            "\0\0\0" // uncompressed size
            "\0\0\0\0" // compressed size
            "\0\0\0\0\0\0\0\0" // pad to 0x10
        ;
        expect( eq( decompress( data, sizeof data ), std::string{} ) );
    };
    
    test( "one block" ) = []
    {
        const char data[] =
            "CDRM" // magic
            "\0\0\0\0" // version
            "\1\0\0\0" // block count
            "\0\0\0\0" // num padding bytes
            // block 0
            "\1" // type=uncompressed
            "\5\0\0" // uncompressed size
            "\5\0\0\0" // compressed size
            "\0\0\0\0\0\0\0\0" // pad to 0x10
            // block 0 data
            "Hello";
        expect( eq( decompress( data, sizeof data ), std::string{"Hello"} ) );
    };

    test( "throws if uncompressed block sizes do not match" ) = []
    {
        const char data[] =
            "CDRM" // magic
            "\0\0\0\0" // version
            "\1\0\0\0" // block count
            "\0\0\0\0" // num padding bytes
            "\1" // type=uncompressed
            "\5\0\0" // uncompressed size=5
            "\4\0\0\0" // compressed size=4
            "\0\0\0\0\0\0\0\0" // pad to 0x10
            "Hell";
        expect( throws( [&]{ decompress( data, sizeof data ); } ) );
    };

    test( "zlib-compressed block" ) = []
    {
        const char data[] =
            "CDRM" // magic
            "\0\0\0\0" // version
            "\1\0\0\0" // block count
            "\0\0\0\0" // num padding bytes
            // block 0
            "\2" // type=zlib
            "\5\0\0" // uncompressed size=5
            "\xD\0\0\0" // compressed size=13
            "\0\0\0\0\0\0\0\0" // pad to 0x10
            // block 0 data: zlib-compressed "Hello"
            "\x78\x9c\xf3\x48\xcd\xc9\xc9\x07\x00\x05\x8c\x01\xf5";
        expect( eq( decompress( data, sizeof data ), std::string{"Hello"} ) );
    };

    test( "throws on corrupt zlib data" ) = []
    {
        const char data[] =
            "CDRM" // magic
            "\0\0\0\0" // version
            "\1\0\0\0" // block count
            "\0\0\0\0" // num padding bytes
            // block 0
            "\2" // type=zlib
            "\5\0\0" // uncompressed size=5
            "\2\0\0\0" // compressed size=2
            "\0\0\0\0\0\0\0\0" // pad to 0x10
            "\0\0"; // invalid zlib data
        expect( throws( [&]{ decompress( data, sizeof data ); } ) );
    };

    test( "throws when zlib output is shorter than declared size" ) = []
    {
        const char data[] =
            "CDRM" // magic
            "\0\0\0\0" // version
            "\1\0\0\0" // block count
            "\0\0\0\0" // num padding bytes
            // block 0
            "\2" // type=zlib
            "\6\0\0" // uncompressed size=6
            "\xD\0\0\0" // compressed size=13
            "\0\0\0\0\0\0\0\0" // pad to 0x10
            // block 0 data: "Hello" (only 5 bytes)
            "\x78\x9c\xf3\x48\xcd\xc9\xc9\x07\x00\x05\x8c\x01\xf5";
        expect( throws( [&]{ decompress( data, sizeof data ); } ) );
    };

    test( "throws if zlib stream is truncated" ) = []
    {
        const char data[] =
            "CDRM" // magic
            "\0\0\0\0" // version
            "\1\0\0\0" // block count
            "\0\0\0\0" // num padding bytes
            "\2" // type=zlib
            "\5\0\0" // uncompressed size=5
            "\xC\0\0\0" // compressed size=12 (one byte truncated)
            "\0\0\0\0\0\0\0\0" // pad to 0x10
            // block 0 data: "Hello" without the "\xf5" at the end
            "\x78\x9c\xf3\x48\xcd\xc9\xc9\x07\x00\x05\x8c\x01";
        expect( throws( [&]{ decompress( data, sizeof data ); } ) );
    };

    test( "throws if decompressed zlib output too long" ) = []
    {
        const char data[] =
            "CDRM" // magic
            "\0\0\0\0" // version
            "\1\0\0\0" // block count
            "\0\0\0\0" // num padding bytes
            "\2" // type=zlib
            "\4\0\0" // uncompressed size=4
            "\xD\0\0\0" // compressed size=13
            "\0\0\0\0\0\0\0\0" // pad to 0x10
            // block 0 data: "Hello" (5 bytes)
            "\x78\x9c\xf3\x48\xcd\xc9\xc9\x07\x00\x05\x8c\x01\xf5";
        expect( throws( [&]{ decompress( data, sizeof data ); } ) );
    };

    test( "throws when zlib block has trailing compressed data" ) = []
    {
        const char data[] =
            "CDRM" // magic
            "\0\0\0\0" // version
            "\1\0\0\0" // block count
            "\0\0\0\0" // num padding bytes
            "\2" // type=zlib
            "\5\0\0" // uncompressed size=5
            "\xE\0\0\0" // compressed size=14
            "\0\0\0\0\0\0\0\0" // pad to 0x10
            // block 0 data: "Hello" with a "\0" at the end
            "\x78\x9c\xf3\x48\xcd\xc9\xc9\x07\x00\x05\x8c\x01\xf5\0";
        expect( throws( [&]{ decompress( data, sizeof data ); } ) );
    };

    test( "multiple blocks" ) = []
    {
        const char data[] =
            "CDRM" // magic
            "\0\0\0\0" // version
            "\2\0\0\0" // block count
            "\0\0\0\0" // num padding bytes
            // block 0
            "\1" // type=uncompressed
            "\1\0\0" // uncompressed size
            "\1\0\0\0" // compressed size
            // block 1
            "\1" // type=uncompressed
            "\1\0\0" // uncompressed size
            "\1\0\0\0" // compressed size
            // block 0 data
            "1"
            "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0" // align to 0x10
            // block 1 data
            "2";
        expect( eq( decompress( data, sizeof data ), std::string{"12"} ) );
    };
};
