#include <catch2/catch_test_macros.hpp>
#include <cdc_lib/resource/rsrc_reloc_input_stream.h>
#include <cdc_lib/resource/rsrc_relocation.h>
#include <score/binary_io/binary_io.h>

TEST_CASE( "resource_reloc_input_stream", "[resource]" )
{
    // NOLINTNEXTLINE(modernize-avoid-c-arrays)
    const char sample_data[] =
        "\xAA\xAA\xAA\xAA" // 00 - magic
        "\x00\x00\x00\x00" // 04 - pointer
        "\xBB\xBB\xBB\xBB" // 08 - pointer dest
    ;
    auto input_interface = score::binary_io::create_input_interface( {sample_data, sizeof sample_data}, std::endian::little );
    auto relocations = std::array< cdc_lib::resource::cooked_relocation, 1 >
    {
        cdc_lib::resource::cooked_relocation{.src_ptr_offset = 0x4, .dest_ptr_offset = 0x8}
    };
    auto stream = cdc_lib::resource::reloc_istream{*input_interface, relocations, 4};

    const auto magic = read< std::uint32_t >( stream );
    SECTION( "read magic" )
    {
        CHECK( magic == 0xAAAAAAAA );
    };

    const auto *const pointer = stream.try_read_relocation();
    SECTION( "scope" )
    {
        {
            auto subsection = cdc_lib::resource::reloc_istream_scope{stream, pointer->dest_ptr_offset, "ptr"};
            CHECK( static_cast< bool >( subsection ) );
            CHECK( read< unsigned >( stream ) == 0xBBBBBBBB );
        }

        SECTION( "subsection" )
        {
            const auto subsections = stream.get_subsections();
            CHECK( subsections.size() == 1 );
            CHECK( subsections[0].bytes_read == 4 );
            CHECK( subsections[0].name == "ptr" );
            CHECK( subsections[0].start_offset == 0x8 );
        };
    };
};
