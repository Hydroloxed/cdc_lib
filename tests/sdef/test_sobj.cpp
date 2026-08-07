#include <boost/ut.hpp>
#include <cdc_lib/sdef/sdef_parse.h>
#include <cdc_lib/sdef/sobj_node.h>
#include <cdc_lib/sdef/sobj_parse.h>
#include <score/binary_io/binary_io.h>

namespace
{
    struct stream_data
    {
        cdc_lib::resource::reloc_istream ristream;
        std::unique_ptr< score::binary_io::input_interface > istream;
        std::string data;
    };
    stream_data setup_stream( const std::string& a_data )
    {
        auto istream = score::binary_io::create_input_interface( a_data );
        auto ristream = cdc_lib::resource::reloc_istream{*istream, {}, 0};
        return {.ristream = std::move( ristream ),
                .istream = std::move( istream ),
                .data = a_data};
    }
}

using namespace boost::ut;

// NOLINTNEXTLINE
boost::ut::suite< "sobj" > sobj = []
{
    test( "can parse empty struct" ) = []
    {
        const std::string data{};
        auto streams = setup_stream( data );
        auto empty_struct_sdef = cdc_lib::sdef::parse_sdef( "<Struct name=\"EmptyStruct\" />" );
        auto sobj = cdc_lib::sdef::parse_sobj( &empty_struct_sdef, streams.ristream );
        expect( eq(sobj.sdef, &empty_struct_sdef) );
        expect( eq(sobj.parent, nullptr) );
        expect( eq(sobj.children.size(), 0) );
    };

    test( "can parse a uint8" ) = []
    {
        const std::string data = "\x01";
        auto streams = setup_stream( data );
        auto sdef = cdc_lib::sdef::parse_sdef( R"(<Struct name="EmptyStruct"><Var name="Var1" type="uint8" /></Struct>)" );
        auto* sdef_var = sdef.children[0].get();
        auto sobj = cdc_lib::sdef::parse_sobj( &sdef, streams.ristream );
        expect( eq(sobj.sdef, &sdef) );
        expect( eq(sobj.parent, nullptr) );
        expect( eq(sobj.children.size(), 1) );
        expect( eq(sobj.children[0]->sdef, sdef_var) );
        expect( eq(sobj.children[0]->var_data.data, 0x01) );
        expect( eq(streams.ristream.tell(), 1) );
    };
};
