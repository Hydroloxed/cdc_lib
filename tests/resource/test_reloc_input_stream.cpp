#include "cdc_lib/resource/rsrc_relocation.h"
#include <boost/ut.hpp>
#include <cdc_lib/resource/rsrc_reloc_input_stream.h>
#include <cdc_lib/resource/tras/pc_w/tras_pc_w_relocation.h>
#include <score/binary_io/binary_io.h>

boost::ut::suite< "resource_reloc_input_stream" > suite_reloc_input_stream = []
{
    using namespace boost::ut;

    // NOLINTNEXTLINE(modernize-avoid-c-arrays)
    const char sample_data[] =
        "\xAA\xAA\xAA\xAA" // 00 - magic
        "\x00\x00\x00\x00" // 04 - pointer
            "\xBB\xBB\xBB\xBB" // 08 - pointer dest
        "\x00\x00\x00\x00" // 0C - null pointer
    ;
    auto input_interface = score::binary_io::create_input_interface( {sample_data, sizeof sample_data}, std::endian::little );
    auto relocations = std::array< cdc_lib::resource::cooked_relocation, 1 >
    {
        cdc_lib::resource::cooked_relocation{.src_ptr_offset = 0x4, .dest_ptr_offset = 0x8}
    };
    auto stream = cdc_lib::resource::reloc_istream{*input_interface, relocations, 4};

    test( "read magic" ) = [&stream]
    {
        expect( read< unsigned >( stream ) == 0xAAAAAAAA );
    };

    test( "scope" ) = [&stream]
    {
        auto subsection = cdc_lib::resource::reloc_istream_scope{stream, "ptr"};
        expect( static_cast< bool >( subsection ) ) << "subsection could be read";
        expect( read< unsigned >( stream ) == 0xBBBBBBBB );
    };

    test( "read null pointer" ) = [&stream]
    {
        auto subsection = cdc_lib::resource::reloc_istream_scope{stream};
        expect( !static_cast< bool >(subsection) ) << "subsection could not be read";
    };

    test( "subsection" ) = [&stream]
    {
        const auto subsections = stream.get_subsections();
        expect( subsections.size() == 1 );
        expect( subsections[0].bytes_read == 4 );
        expect( subsections[0].name == "ptr" );
        expect( subsections[0].start_offset == 0x8 );
    };
};
