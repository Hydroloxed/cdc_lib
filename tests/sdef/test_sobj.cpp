#include <boost/ut.hpp>
#include <cdc_lib/sdef/sdef_parse.h>
#include <cdc_lib/sdef/sobj_node.h>
#include <cdc_lib/sdef/sobj_parse.h>
#include <limits>
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
        expect( eq(sobj.children[0]->var_data.as_uint(), 0x01) );
        expect( eq(streams.ristream.tell(), 1) );
    };

    test( "can parse a uint16" ) = []
    {
        const std::string data = "\xff\xff";
        auto streams = setup_stream( data );
        auto sdef = cdc_lib::sdef::parse_sdef( R"(<Struct name="EmptyStruct"><Var name="Var1" type="uint16" /></Struct>)" );
        auto sobj = cdc_lib::sdef::parse_sobj( &sdef, streams.ristream );
        expect( eq(sobj.children[0]->var_data.as_uint(), std::numeric_limits<std::uint16_t>::max()) );
    };

    test( "can parse a uint32" ) = []
    {
        const std::string data = "\xff\xff\xff\xff";
        auto streams = setup_stream( data );
        auto sdef = cdc_lib::sdef::parse_sdef( R"(<Struct name="EmptyStruct"><Var name="Var1" type="uint32" /></Struct>)" );
        auto sobj = cdc_lib::sdef::parse_sobj( &sdef, streams.ristream );
        expect( eq(sobj.children[0]->var_data.as_uint(), std::numeric_limits<std::uint32_t>::max()) );
    };

    test( "can parse a uint64" ) = []
    {
        const std::string data = "\xff\xff\xff\xff\xff\xff\xff\xff";
        auto streams = setup_stream( data );
        auto sdef = cdc_lib::sdef::parse_sdef( R"(<Struct name="EmptyStruct"><Var name="Var1" type="uint64" /></Struct>)" );
        auto sobj = cdc_lib::sdef::parse_sobj( &sdef, streams.ristream );
        expect( eq(sobj.children[0]->var_data.as_uint(), std::numeric_limits<std::uint64_t>::max()) );
    };

    test( "can parse a int8" ) = []
    {
        const std::string data = "\xff";
        auto streams = setup_stream( data );
        auto sdef = cdc_lib::sdef::parse_sdef( R"(<Struct name="EmptyStruct"><Var name="Var1" type="int8" /></Struct>)" );
        auto sobj = cdc_lib::sdef::parse_sobj( &sdef, streams.ristream );
        expect( eq(sobj.children[0]->var_data.as_int(), -1) );
    };

    test( "can parse a int16" ) = []
    {
        const std::string data = "\xff\xff";
        auto streams = setup_stream( data );
        auto sdef = cdc_lib::sdef::parse_sdef( R"(<Struct name="EmptyStruct"><Var name="Var1" type="int16" /></Struct>)" );
        auto sobj = cdc_lib::sdef::parse_sobj( &sdef, streams.ristream );
        expect( eq(sobj.children[0]->var_data.as_int(), -1) );
    };

    test( "can parse a int32" ) = []
    {
        const std::string data = "\xff\xff\xff\xff";
        auto streams = setup_stream( data );
        auto sdef = cdc_lib::sdef::parse_sdef( R"(<Struct name="EmptyStruct"><Var name="Var1" type="int32" /></Struct>)" );
        auto sobj = cdc_lib::sdef::parse_sobj( &sdef, streams.ristream );
        expect( eq(sobj.children[0]->var_data.as_int(), -1) );
    };

    test( "can parse a int32" ) = []
    {
        const std::string data = "\xff\xff\xff\xff\xff\xff\xff\xff";
        auto streams = setup_stream( data );
        auto sdef = cdc_lib::sdef::parse_sdef( R"(<Struct name="EmptyStruct"><Var name="Var1" type="int64" /></Struct>)" );
        auto sobj = cdc_lib::sdef::parse_sobj( &sdef, streams.ristream );
        expect( eq(sobj.children[0]->var_data.as_int(), -1) );
    };
};
