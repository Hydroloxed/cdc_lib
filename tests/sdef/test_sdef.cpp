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

    test( "element names are case-insensitive") = []
    {
        const auto sdef_tree = cdc_lib::sdef::parse_sdef( "<Struct name=\"SomeSuperStruct\"></Struct>" );
        expect( sdef_tree.is_struct() );
        expect( sdef_tree.as_struct().name == "SomeSuperStruct" );
    };

    test( "can add a var" ) = []
    {
        const auto sdef_tree = cdc_lib::sdef::parse_sdef( R"(<struct name="SomeSuperStruct"><var name="SomeVar" type="int" /></struct>)" );
        expect( sdef_tree.is_struct() );
        expect( sdef_tree.as_struct().name == "SomeSuperStruct" );
        expect( sdef_tree.children.size() == 1 );
        expect( sdef_tree.children[0]->is_var() );
        expect( sdef_tree.children[0]->as_var().name == "SomeVar" );
        expect( sdef_tree.children[0]->as_var().type.type_name == "int" );
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
            expect( sdef_tree.children[0]->as_var().type.primitive_type == primitive_type ) << type;
        }
    };
};
