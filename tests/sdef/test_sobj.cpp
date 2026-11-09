#include "cdc_lib/resource/rsrc_relocation.h"
#include <catch2/catch_test_macros.hpp>
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
    stream_data setup_stream( const std::string& a_data,
                              std::span< cdc_lib::resource::cooked_relocation > a_relocations = {},
                              std::size_t a_pointer_size = 4 )
    {
        auto istream = score::binary_io::create_input_interface( a_data );
        auto ristream = cdc_lib::resource::reloc_istream{*istream, a_relocations, a_pointer_size};
        return {.ristream = std::move( ristream ),
                .istream = std::move( istream ),
                .data = a_data};
    }
}

TEST_CASE( "sobj", "[sdef]" )
{
    SECTION( "can parse empty struct" )
    {
        const std::string data{};
        auto streams = setup_stream( data );
        auto empty_struct_sdef = cdc_lib::sdef::parse_sdef( "<Struct name=\"EmptyStruct\" />" );
        auto sobj = cdc_lib::sdef::parse_sobj( &empty_struct_sdef, streams.ristream );
        CHECK(sobj.sdef == &empty_struct_sdef);
        CHECK(sobj.parent == nullptr);
        CHECK(sobj.children.size() == 0);
    };

    SECTION( "can parse a uint8" )
    {
        const std::string data = "\x01";
        auto streams = setup_stream( data );
        auto sdef = cdc_lib::sdef::parse_sdef( R"(<Struct name="EmptyStruct"><Var name="Var1" type="uint8" /></Struct>)" );
        auto* sdef_var = sdef.children.at(0).get();
        auto sobj = cdc_lib::sdef::parse_sobj( &sdef, streams.ristream );
        CHECK(sobj.sdef == &sdef);
        CHECK(sobj.parent == nullptr);
        CHECK(sobj.children.size() == 1);
        CHECK(sobj.children.at(0)->sdef == sdef_var);
        CHECK(sobj.children.at(0)->var_data.as_uint() == 0x01);
        CHECK(streams.ristream.tell() == 1);
    };

    SECTION( "can parse a uint16" )
    {
        const std::string data = "\xff\xff";
        auto streams = setup_stream( data );
        auto sdef = cdc_lib::sdef::parse_sdef( R"(<Struct name="EmptyStruct"><Var name="Var1" type="uint16" /></Struct>)" );
        auto sobj = cdc_lib::sdef::parse_sobj( &sdef, streams.ristream );
        CHECK(sobj.children.at(0)->var_data.as_uint() == std::numeric_limits<std::uint16_t>::max());
    };

    SECTION( "can parse a uint32" )
    {
        const std::string data = "\xff\xff\xff\xff";
        auto streams = setup_stream( data );
        auto sdef = cdc_lib::sdef::parse_sdef( R"(<Struct name="EmptyStruct"><Var name="Var1" type="uint32" /></Struct>)" );
        auto sobj = cdc_lib::sdef::parse_sobj( &sdef, streams.ristream );
        CHECK(sobj.children.at(0)->var_data.as_uint() == std::numeric_limits<std::uint32_t>::max());
    };

    SECTION( "can parse a uint64" )
    {
        const std::string data = "\xff\xff\xff\xff\xff\xff\xff\xff";
        auto streams = setup_stream( data );
        auto sdef = cdc_lib::sdef::parse_sdef( R"(<Struct name="EmptyStruct"><Var name="Var1" type="uint64" /></Struct>)" );
        auto sobj = cdc_lib::sdef::parse_sobj( &sdef, streams.ristream );
        CHECK(sobj.children.at(0)->var_data.as_uint() == std::numeric_limits<std::uint64_t>::max());
    };

    SECTION( "can parse a int8" )
    {
        const std::string data = "\xff";
        auto streams = setup_stream( data );
        auto sdef = cdc_lib::sdef::parse_sdef( R"(<Struct name="EmptyStruct"><Var name="Var1" type="int8" /></Struct>)" );
        auto sobj = cdc_lib::sdef::parse_sobj( &sdef, streams.ristream );
        CHECK(sobj.children.at(0)->var_data.as_int() == -1);
    };

    SECTION( "can parse a int16" )
    {
        const std::string data = "\xff\xff";
        auto streams = setup_stream( data );
        auto sdef = cdc_lib::sdef::parse_sdef( R"(<Struct name="EmptyStruct"><Var name="Var1" type="int16" /></Struct>)" );
        auto sobj = cdc_lib::sdef::parse_sobj( &sdef, streams.ristream );
        CHECK(sobj.children.at(0)->var_data.as_int() == -1);
    };

    SECTION( "can parse a int32" )
    {
        const std::string data = "\xff\xff\xff\xff";
        auto streams = setup_stream( data );
        auto sdef = cdc_lib::sdef::parse_sdef( R"(<Struct name="EmptyStruct"><Var name="Var1" type="int32" /></Struct>)" );
        auto sobj = cdc_lib::sdef::parse_sobj( &sdef, streams.ristream );
        CHECK(sobj.children.at(0)->var_data.as_int() == -1);
    };

    SECTION( "can parse a int32" )
    {
        const std::string data = "\xff\xff\xff\xff\xff\xff\xff\xff";
        auto streams = setup_stream( data );
        auto sdef = cdc_lib::sdef::parse_sdef( R"(<Struct name="EmptyStruct"><Var name="Var1" type="int64" /></Struct>)" );
        auto sobj = cdc_lib::sdef::parse_sobj( &sdef, streams.ristream );
        CHECK(sobj.children.at(0)->var_data.as_int() == -1);
    };

    SECTION( "can parse a bool8 - 1 is true" )
    {
        const std::string data = "\x01";
        auto streams = setup_stream( data );
        auto sdef = cdc_lib::sdef::parse_sdef( R"(<Struct name="EmptyStruct"><Var name="Var1" type="bool8" /></Struct>)" );
        auto sobj = cdc_lib::sdef::parse_sobj( &sdef, streams.ristream );
        CHECK(sobj.children.at(0)->var_data.as_bool() == true);
    };

    SECTION( "can parse a bool8 - 0 is false" )
    {
        const std::string data = std::string{"\x00", 1};
        auto streams = setup_stream( data );
        auto sdef = cdc_lib::sdef::parse_sdef( R"(<Struct name="EmptyStruct"><Var name="Var1" type="bool8" /></Struct>)" );
        auto sobj = cdc_lib::sdef::parse_sobj( &sdef, streams.ristream );
        CHECK(sobj.children.at(0)->var_data.as_bool() == false);
    };

    SECTION( "can parse a bool8 - other values are true" )
    {
        const std::string data = "\x12";
        auto streams = setup_stream( data );
        auto sdef = cdc_lib::sdef::parse_sdef( R"(<Struct name="EmptyStruct"><Var name="Var1" type="bool8" /></Struct>)" );
        auto sobj = cdc_lib::sdef::parse_sobj( &sdef, streams.ristream );
        CHECK(sobj.children.at(0)->var_data.as_bool() == true);
    };

    SECTION( "can parse a float32" )
    {
        const std::string data = std::string{"\x00\x00\xf6\x42", 4};
        auto streams = setup_stream( data );
        auto sdef = cdc_lib::sdef::parse_sdef( R"(<Struct name="EmptyStruct"><Var name="Var1" type="float32" /></Struct>)" );
        auto sobj = cdc_lib::sdef::parse_sobj( &sdef, streams.ristream );
        CHECK(sobj.children.at(0)->var_data.as_float32() == 123.0f);
    };

    SECTION( "can parse a float64" )
    {
        const std::string data = std::string{"\x00\x00\x00\x00\x00\xc0\x5e\x40", 8};
        auto streams = setup_stream( data );
        auto sdef = cdc_lib::sdef::parse_sdef( R"(<Struct name="EmptyStruct"><Var name="Var1" type="float64" /></Struct>)" );
        auto sobj = cdc_lib::sdef::parse_sobj( &sdef, streams.ristream );
        CHECK(sobj.children.at(0)->var_data.as_float64() == 123.0);
    };

    const cdc_lib::resource::cooked_relocation k_relocation =
            {.src_ptr_offset = 0,
             .dest_ptr_offset = 4,
             .type = cdc_lib::resource::cooked_relocation_type::internal};

    SECTION( "can parse a string" )
    {
        const std::string data = std::string{"\x00\x00\x00\x00hello\x00", 10};
        std::vector relocations{k_relocation};
        auto streams = setup_stream( data, relocations, 4 );
        auto sdef = cdc_lib::sdef::parse_sdef( R"(<Struct name="EmptyStruct"><Var name="Var1" type="string" /></Struct>)" );
        auto sobj = cdc_lib::sdef::parse_sobj( &sdef, streams.ristream );
        CHECK(sobj.children.at(0)->var_data.as_string() == "hello");
        CHECK(streams.ristream.tell() == 4);
    };

    SECTION( "can parse a null string" )
    {
        const std::string data = std::string{"\x00\x00\x00\x00\x00", 5};
        auto streams = setup_stream( data, {}, 4 );
        auto sdef = cdc_lib::sdef::parse_sdef( R"(<Struct name="EmptyStruct"><Var name="Var1" type="string" /></Struct>)" );
        auto sobj = cdc_lib::sdef::parse_sobj( &sdef, streams.ristream );
        CHECK(sobj.children.at(0)->var_data.as_string() == "");
    };

    SECTION( "can parse an empty string" )
    {
        const std::string data = std::string{"\x00\x00\x00\x00\x00", 5};
        std::vector relocations{k_relocation};
        auto streams = setup_stream( data, relocations, 4 );
        auto sdef = cdc_lib::sdef::parse_sdef( R"(<Struct name="EmptyStruct"><Var name="Var1" type="string" /></Struct>)" );
        auto sobj = cdc_lib::sdef::parse_sobj( &sdef, streams.ristream );
        CHECK(sobj.children.at(0)->var_data.as_string() == "");
    };

    SECTION( "enums are parsed as their underlying type" )
    {
        const std::string data = std::string{"\x12\x34", 2};
        auto streams = setup_stream( data );
        auto sdef = cdc_lib::sdef::parse_sdef( R"(
            <Struct name="XYZ">
                <Enum name="SomeSuperEnum" underlying="uint16"></Enum>
                <Var name="SomeSuperVariable" type="SomeSuperEnum" />
            </Struct>)" );
        auto sobj = cdc_lib::sdef::parse_sobj( &sdef, streams.ristream );
        CHECK(sobj.children.at(0)->var_data.as_uint() == 0x3412);
    };

    SECTION( "can parse nested structs" )
    {
        const std::string data = std::string{"\x12\x34", 2};
        auto streams = setup_stream( data );
        auto sdef = cdc_lib::sdef::parse_sdef( R"(
            <Struct name="Struct1">
                <Struct name="Struct2">
                    <Var name="Var3" type="uint8" />
                </Struct>
                <Var name="Var1" type="uint8" />
                <Var name="Var2" type="Struct2" />
            </Struct>)" );
        auto sobj = cdc_lib::sdef::parse_sobj( &sdef, streams.ristream );
        const auto* sdef_var1 = sdef.children.at(1).get();
        const auto* sdef_var2 = sdef.children.at(2).get();
        REQUIRE(sobj.children.size() == 2);
        REQUIRE(sobj.children.at(0)->children.size() == 0);
        CHECK(sobj.children.at(0)->var_data.as_uint() == 0x12);
        CHECK(sobj.children.at(0)->sdef == sdef_var1);
        REQUIRE(sobj.children.at(1)->children.size() == 1);
        CHECK(sobj.children.at(1)->children.at(0)->var_data.as_uint() == 0x34);
        CHECK(sobj.children.at(1)->sdef == sdef_var2);
    };
};
