#include "cdc_lib/sdef/sdef_type.h"
#include <boost/ut.hpp>
#include <cdc_lib/sdef/sdef_parse.h>

using namespace boost::ut;

// NOLINTNEXTLINE
boost::ut::suite< "sdef" > sdef = []
{
    test( "can't have empty file") = []
    {
        expect( throws( []{ cdc_lib::sdef::parse_sdef( "" ); } ) );
    };

    test( "rejects invalid xml" ) = []
    {
        expect( throws( []{ cdc_lib::sdef::parse_sdef( "<struct / \"" ); } ) );
        expect( throws( []{ cdc_lib::sdef::parse_sdef( "<struct name=\"abc\"></struct> < / \"" ); } ) );
    };

    test( "rejects unknown elements" ) = []
    {
        expect( throws( []{ cdc_lib::sdef::parse_sdef( "<IMadeThisElementUp/>" ); } ) );
    };

    test( "modules need a name and a namespace" ) = []
    {
        expect( throws( []{ cdc_lib::sdef::parse_sdef( "<module/>" ); } ) );
        expect( throws( []{ cdc_lib::sdef::parse_sdef( "<module name=\"ABC123\"/>" ); } ) );
        expect( throws( []{ cdc_lib::sdef::parse_sdef( "<module namespace=\"cdc\"/>" ); } ) );
    };

    test( "can add a module" ) = []
    {
        const auto sdef_tree = cdc_lib::sdef::parse_sdef( R"(
            <module name="SomeModule" namespace="cdc">
            </module>)" );
        expect( sdef_tree.is_module() );
        expect( eq( sdef_tree.as_module().name, std::string{"SomeModule"} ) );
        expect( eq( sdef_tree.as_module().cpp_namespace, std::string{"cdc"} ) );
    };

    test( "can't have nested modules" ) = []
    {
        expect( throws( []{ cdc_lib::sdef::parse_sdef( R"(
            <module name="SomeModule" namespace="cdc">
                <module name="SomeOtherModule" namespace="cdc">
                </module>
            </module>)" ); } ) );
    };

    test( "can't have multiple root nodes" ) = []
    {
        expect( throws( []{ cdc_lib::sdef::parse_sdef( "<struct /><struct />" ); } ) );
    };

    test( "a struct can't have an empty name" ) = []
    {
        expect( throws( []{ cdc_lib::sdef::parse_sdef( "<struct></struct>" ); } ) );
    };

    test( "can add an empty struct correctly" ) = []
    {
        const auto sdef_tree = cdc_lib::sdef::parse_sdef( "<struct name=\"SomeSuperStruct\"></struct>" );
        expect( sdef_tree.is_struct() );
        expect( sdef_tree.as_struct().name == "SomeSuperStruct" );
    };

    test( "sets parent correctly" ) = []
    {
        const auto sdef_tree = cdc_lib::sdef::parse_sdef( "<struct name=\"SomeSuperStruct\"><var name=\"SomeVar\" type=\"int32\" /></struct>" );
        expect( sdef_tree.is_struct() );
        expect( eq(sdef_tree.children.at(0)->parent, &sdef_tree) );
    };

    test( "element names are case-insensitive") = []
    {
        const auto sdef_tree = cdc_lib::sdef::parse_sdef( "<Struct name=\"SomeSuperStruct\"></Struct>" );
        expect( sdef_tree.is_struct() );
        expect( sdef_tree.as_struct().name == "SomeSuperStruct" );
    };

    test( "can add a var" ) = []
    {
        const auto sdef_tree = cdc_lib::sdef::parse_sdef( R"(<struct name="SomeSuperStruct"><var name="SomeVar" type="int32" /></struct>)" );
        expect( sdef_tree.is_struct() );
        expect( sdef_tree.as_struct().name == "SomeSuperStruct" );
        expect( sdef_tree.children.size() == 1 );
        expect( sdef_tree.children.at(0)->is_var() );
        expect( sdef_tree.children.at(0)->as_var().name == "SomeVar" );
        expect( sdef_tree.children.at(0)->as_var().type.type_name == "int32" );
    };

    test( "a var needs a name and a type" ) = []
    {
        expect( throws( []{ cdc_lib::sdef::parse_sdef( R"(<struct name="SomeSuperStruct"><var /></struct>)" ); } ) );
        expect( throws( []{ cdc_lib::sdef::parse_sdef( R"(<struct name="SomeSuperStruct"><var name="SomeVar" /></struct>)" ); } ) );
        expect( throws( []{ cdc_lib::sdef::parse_sdef( R"(<struct name="SomeSuperStruct"><var type="SomeType" /></struct>)" ); } ) );
    };

    test( "variables can have primitive types" ) = []
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
            expect( sdef_tree.children.at(0)->as_var().type.primitive_type == primitive_type ) << type;
        }
    };

    test( "throws on invalid variable types" ) = []
    {
        expect( throws( []{ cdc_lib::sdef::parse_sdef( R"(<struct name="SomeSuperStruct"><var name="SomeVar" type="MadeUpType"/></struct>)" ); } ) );
    };

    test( "an enum needs a name and an underlying type" ) = []
    {
        expect( throws( []{ cdc_lib::sdef::parse_sdef( R"(<Enum></Enum>)" ); } ) );
        expect( throws( []{ cdc_lib::sdef::parse_sdef( R"(<Enum name="SomeSuperEnum"></Enum>)" ); } ) );
        expect( throws( []{ cdc_lib::sdef::parse_sdef( R"(<Enum underlying="uint32"></Enum>)" ); } ) );
    };

    test( "an enum must have a valid underlying type" ) = []
    {
        expect( throws( []{ cdc_lib::sdef::parse_sdef( R"(<Enum name="SomeSuperEnum" underlying="uint651243"></Enum>)" ); } ) );
    };

    test( "enums must be declared before use" ) = []
    {
        expect( throws( []{ cdc_lib::sdef::parse_sdef( R"(
            <Struct name="SomeSuperStruct">
                <Var name="SomeSuperVar" type="SomeSuperEnum" />
                <Enum name="SomeSuperEnum" underlying="uint651243">
                </Enum>
            </Struct>)" ); } ) );
    };


    test( "can add an empty enum correctly" ) = []
    {
        using namespace cdc_lib::sdef;

        const auto sdef_tree = cdc_lib::sdef::parse_sdef( R"(<Enum name="SomeSuperEnum" underlying="uint32"></Enum>)" );
        expect( sdef_tree.is_enum() );
        expect( sdef_tree.as_enum().name == "SomeSuperEnum" );
        expect( sdef_tree.as_enum().underlying_type == sdef_primitive_type::uint32 );
        expect( sdef_tree.children.empty() );
    };

    test( "enums types are resolved correctly" ) = []
    {
        using namespace cdc_lib::sdef;

        const auto sdef_tree = cdc_lib::sdef::parse_sdef( R"(
            <Struct name="XYZ">
                <Enum name="SomeSuperEnum" underlying="uint32"></Enum>
                <Var name="SomeSuperVariable" type="SomeSuperEnum" />
            </Struct>)" );
        expect( eq(sdef_tree.children.at(0)->as_enum().name, std::string{"SomeSuperEnum"}) );
        expect( eq(sdef_tree.children.at(1)->as_var().name, std::string{"SomeSuperVariable"}) );
        expect( eq(sdef_tree.children.at(1)->as_var().type.type_name, std::string{"SomeSuperEnum"}) );
        expect( sdef_tree.children.at(1)->as_var().type.primitive_type == sdef_primitive_type::none );
        expect( eq(sdef_tree.children.at(1)->as_var().type.type, &*sdef_tree.children.at(0)) );
    };
};
