#include <cdc_lib/sdef/sdef_parse.h>
#include <cxxopts.hpp>
#include <fmt/core.h>
#include <fstream>

std::string indent( int a_level )
{
    return std::string( a_level * 2, ' ' );
}

void tree_dump( cdc_lib::sdef::sdef_node& a_node, int a_indent = 0 )
{
    if( a_node.is_struct() )
        fmt::print( "{}struct {}\n", indent( a_indent ), a_node.as_struct().name );
    for( const auto& child : a_node.children )
        tree_dump( *child, a_indent + 1 );
}

int main( int argc, char** argv )
{
    auto options = cxxopts::Options{"sdef_compile", "Compile an SDEF file"};
    options.add_options()
        ( "h,help", "Print help" )
        ( "i,input", "Input SDEF file", cxxopts::value< std::string >() )
        ( "f,output-format",
          "Output format (valid: treedump), default treedump",
          cxxopts::value< std::string >()->default_value( "treedump" ) );
    options.parse_positional( {"input"} );
    options.positional_help( "<input>" );

    auto result = options.parse( argc, argv );
    if( argc < 2 || result.contains("help") )
    {
        fmt::print( "{}\n", options.help() );
        return EXIT_FAILURE;
    }
    const auto filename = result["input"].as< std::string >();
    std::ifstream input_file{filename};
    // input_file.exceptions( std::ios::badbit | std::ios::eofbit | std::ios::failbit );
    const auto input_size = std::filesystem::file_size( filename );
    std::string input_data( input_size, '\0'  );
    input_file.read( input_data.data(), static_cast< std::streamsize >( input_size ) );
    const auto sdef_tree = cdc_lib::sdef::parse_sdef( input_data );

    return EXIT_SUCCESS;
}
