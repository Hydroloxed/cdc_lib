#include <cdc_lib/resource/tras/pc_w/tras_pc_w_relocation.h>
#include <cxxopts.hpp>
#include <filesystem>
#include <fmt/core.h>
#include <fstream>
#include <iterator>
#include <map>
#include <numeric>
#include <score/binary_io/binary_io.h>

int main( int argc, char** argv )
{
    auto options = cxxopts::Options{"section_analyze_subsections", "Try to guess the subsections present in a file."};
    options.add_options()
        ( "file", "The file to analyze", cxxopts::value< std::string >() );
    options.parse_positional( {"file"} );
    options.positional_help( "<file>" );
    auto result = options.parse( argc, argv );
    if( argc < 2 )
    {
        fmt::print( stderr, "{}\n", options.help() );
        return EXIT_FAILURE;
    }
    const auto filename = result["file"].as< std::string >();
    auto stream = std::ifstream{filename};
    if( !stream.good() )
    {
        fmt::print( stderr, "Could not open file '{}'\n", filename );
        return EXIT_FAILURE;
    }
    stream.exceptions( std::ifstream::failbit | std::ifstream::badbit );
    auto i_interface = score::binary_io::create_input_interface( stream );
    auto relocations = cdc_lib::resource::tras::pc_w::load_relocation_table( *i_interface );
    const auto file_size = std::filesystem::file_size( filename );
    const auto data_start = i_interface->tell();
    fmt::print( "relocations size: {:4x}\n", data_start );

    using offset = std::size_t;
    std::map< offset, std::size_t > subsections{{0u, 0u}};
    for( const auto& relocation : relocations )
        if( relocation.is_internal() )
            subsections.insert( {relocation.dest_ptr_offset, 0u} );

    // imaginary 'end-of-file' subsection, needed to get the last subsection size
    subsections.insert( {file_size - data_start, 0u} );

    // set second value (size) to difference between adjacent offsets
    auto prev_it = subsections.begin();
    for( auto it = std::next( subsections.begin() ); it != subsections.end(); prev_it = it++ )
        prev_it->second = it->first - prev_it->first;

    // remove imaginary subsection
    subsections.erase( std::prev( subsections.end() ) );
    for( const auto& subsection : subsections )
        fmt::print( "sub {:6x} size {:4x}\n", subsection.first, subsection.second );
    return EXIT_SUCCESS;
}