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
        expect( sdef_tree.children[0]->as_var().type.type == "int" );
    };

    test( "a var needs a name and a type" ) = []
    {
        expect( throws( []{ cdc_lib::sdef::parse_sdef( R"(<struct name="SomeSuperStruct"><var /></struct>)" ); } ) );
        expect( throws( []{ cdc_lib::sdef::parse_sdef( R"(<struct name="SomeSuperStruct"><var name="SomeVar" /></struct>)" ); } ) );
        expect( throws( []{ cdc_lib::sdef::parse_sdef( R"(<struct name="SomeSuperStruct"><var type="SomeType" /></struct>)" ); } ) );
    };
};
