#include "cdc_lib/sdef/sdef_type.h"
#include <catch2/catch_test_macros.hpp>
#include <cdc_lib/sdef/sdef_parse.h>

TEST_CASE( "sdef", "[sdef]" )
{
    SECTION( "can't have empty file")
    {
        CHECK_THROWS( cdc_lib::sdef::parse_sdef( "" ) );
    };

    SECTION( "rejects invalid xml" )
    {
        CHECK_THROWS( cdc_lib::sdef::parse_sdef( "<struct / \"" ) );
        CHECK_THROWS( cdc_lib::sdef::parse_sdef( "<struct name=\"abc\"></struct> < / \"" ) );
    };

    SECTION( "rejects unknown elements" )
    {
        CHECK_THROWS( cdc_lib::sdef::parse_sdef( "<IMadeThisElementUp/>" ) );
    };

    SECTION( "modules need a name and a namespace" )
    {
        CHECK_THROWS( cdc_lib::sdef::parse_sdef( "<module/>" ) );
        CHECK_THROWS( cdc_lib::sdef::parse_sdef( "<module name=\"ABC123\"/>" ) );
        CHECK_THROWS( cdc_lib::sdef::parse_sdef( "<module namespace=\"cdc\"/>" ) );
    };

    SECTION( "can add a module" )
    {
        const auto sdef_tree = cdc_lib::sdef::parse_sdef( R"(
            <module name="SomeModule" namespace="cdc">
            </module>)" );
        CHECK( sdef_tree.is_module() );
        CHECK( sdef_tree.as_module().name == "SomeModule" );
        CHECK( sdef_tree.as_module().cpp_namespace == "cdc" );
    };

    SECTION( "can't have nested modules" )
    {
        CHECK_THROWS( cdc_lib::sdef::parse_sdef( R"(
            <module name="SomeModule" namespace="cdc">
                <module name="SomeOtherModule" namespace="cdc">
                </module>
            </module>)" ) );
    };

    SECTION( "can't have multiple root nodes" )
    {
        CHECK_THROWS( cdc_lib::sdef::parse_sdef( "<struct /><struct />" ) );
    };

    SECTION( "a struct can't have an empty name" )
    {
        CHECK_THROWS( cdc_lib::sdef::parse_sdef( "<struct></struct>" ) );
    };

    SECTION( "can add an empty struct correctly" )
    {
        const auto sdef_tree = cdc_lib::sdef::parse_sdef( "<struct name=\"SomeSuperStruct\"></struct>" );
        CHECK( sdef_tree.is_struct() );
        CHECK( sdef_tree.as_struct().name == "SomeSuperStruct" );
    };

    SECTION( "sets parent correctly" )
    {
        const auto sdef_tree = cdc_lib::sdef::parse_sdef( "<struct name=\"SomeSuperStruct\"><var name=\"SomeVar\" type=\"int32\" /></struct>" );
        CHECK( sdef_tree.is_struct() );
        CHECK( sdef_tree.children.at(0)->parent == &sdef_tree );
    };

    SECTION( "element names are case-insensitive")
    {
        const auto sdef_tree = cdc_lib::sdef::parse_sdef( "<Struct name=\"SomeSuperStruct\"></Struct>" );
        CHECK( sdef_tree.is_struct() );
        CHECK( sdef_tree.as_struct().name == "SomeSuperStruct" );
    };

    SECTION( "can add a var" )
    {
        const auto sdef_tree = cdc_lib::sdef::parse_sdef( R"(<struct name="SomeSuperStruct"><var name="SomeVar" type="int32" /></struct>)" );
        CHECK( sdef_tree.is_struct() );
        CHECK( sdef_tree.as_struct().name == "SomeSuperStruct" );
        CHECK( sdef_tree.children.size() == 1 );
        CHECK( sdef_tree.children.at(0)->is_var() );
        CHECK( sdef_tree.children.at(0)->as_var().name == "SomeVar" );
        CHECK( sdef_tree.children.at(0)->as_var().type.type_name == "int32" );
    };

    SECTION( "a var needs a name and a type" )
    {
        CHECK_THROWS( cdc_lib::sdef::parse_sdef( R"(<struct name="SomeSuperStruct"><var /></struct>)" ) );
        CHECK_THROWS( cdc_lib::sdef::parse_sdef( R"(<struct name="SomeSuperStruct"><var name="SomeVar" /></struct>)" ) );
        CHECK_THROWS( cdc_lib::sdef::parse_sdef( R"(<struct name="SomeSuperStruct"><var type="SomeType" /></struct>)" ) );
    };

    SECTION( "variables can have primitive types" )
    {
        using namespace cdc_lib::sdef;
        const std::vector types = {std::pair{"bool8", sdef_primitive_type::bool8},
                                   std::pair{"int8", sdef_primitive_type::int8},
                                   std::pair{"uint8", sdef_primitive_type::uint8},
                                   std::pair{"int16", sdef_primitive_type::int16},
                                   std::pair{"uint16", sdef_primitive_type::uint16},
                                   std::pair{"int32", sdef_primitive_type::int32},
                                   std::pair{"uint32", sdef_primitive_type::uint32},
                                   std::pair{"int64", sdef_primitive_type::int64},
                                   std::pair{"uint64", sdef_primitive_type::uint64},
                                   std::pair{"float32", sdef_primitive_type::float32},
                                   std::pair{"float64", sdef_primitive_type::float64},
                                   std::pair{"string", sdef_primitive_type::string},};
        for( const auto& [type, primitive_type] : types )
        {
            std::string sdef_str = std::string{R"(<struct name="SomeSuperStruct"><var name="SomeVar" type=")"} + type + R"(" /></struct>)";
            const auto sdef_tree = parse_sdef( sdef_str );
            CHECK( sdef_tree.children.at(0)->as_var().type.primitive_type == primitive_type );
        }
    };

    SECTION( "throws on invalid variable types" )
    {
        CHECK_THROWS( cdc_lib::sdef::parse_sdef( R"(<struct name="SomeSuperStruct"><var name="SomeVar" type="MadeUpType"/></struct>)" ) );
    };

    SECTION( "an enum needs a name and an underlying type" )
    {
        CHECK_THROWS( cdc_lib::sdef::parse_sdef( R"(<Enum></Enum>)" ) );
        CHECK_THROWS( cdc_lib::sdef::parse_sdef( R"(<Enum name="SomeSuperEnum"></Enum>)" ) );
        CHECK_THROWS( cdc_lib::sdef::parse_sdef( R"(<Enum underlying="uint32"></Enum>)" ) );
    };

    SECTION( "an enum must have a valid underlying type" )
    {
        CHECK_THROWS( cdc_lib::sdef::parse_sdef( R"(<Enum name="SomeSuperEnum" underlying="uint651243"></Enum>)" ) );
    };

    SECTION( "enums must be declared before use" )
    {
        CHECK_THROWS( cdc_lib::sdef::parse_sdef( R"(
            <Struct name="SomeSuperStruct">
                <Var name="SomeSuperVar" type="SomeSuperEnum" />
                <Enum name="SomeSuperEnum" underlying="uint651243">
                </Enum>
            </Struct>)" ) );
    };


    SECTION( "can add an empty enum correctly" )
    {
        using namespace cdc_lib::sdef;

        const auto sdef_tree = cdc_lib::sdef::parse_sdef( R"(<Enum name="SomeSuperEnum" underlying="uint32"></Enum>)" );
        CHECK( sdef_tree.is_enum() );
        CHECK( sdef_tree.as_enum().name == "SomeSuperEnum" );
        CHECK( sdef_tree.as_enum().underlying_type == sdef_primitive_type::uint32 );
        CHECK( sdef_tree.children.empty() );
    };

    SECTION( "enums types are resolved correctly" )
    {
        using namespace cdc_lib::sdef;

        const auto sdef_tree = cdc_lib::sdef::parse_sdef( R"(
            <Struct name="XYZ">
                <Enum name="SomeSuperEnum" underlying="uint32"></Enum>
                <Var name="SomeSuperVariable" type="SomeSuperEnum" />
            </Struct>)" );
        CHECK( sdef_tree.children.at(0)->as_enum().name == "SomeSuperEnum" );
        CHECK( sdef_tree.children.at(1)->as_var().name == "SomeSuperVariable" );
        CHECK( sdef_tree.children.at(1)->as_var().type.type_name == "SomeSuperEnum" );
        CHECK( sdef_tree.children.at(1)->as_var().type.primitive_type == sdef_primitive_type::none );
        CHECK( sdef_tree.children.at(1)->as_var().type.type == &*sdef_tree.children.at(0) );
    };
};
