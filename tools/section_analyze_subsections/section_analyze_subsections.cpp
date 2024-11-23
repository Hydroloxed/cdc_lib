#include <cdc_lib/resource/tras/pc_w/tras_pc_w_relocation.h>
#include <cxxopts.hpp>
#include <filesystem>
#include <fmt/core.h>
#include <fstream>
#include <iterator>
#include <map>
#include <score/binary_io/binary_io.h>
#include <score/binary_io/binio_strings.h>

std::multimap< std::size_t, std::string > load_types_map( std::ifstream&& a_stream )
{
    std::multimap< std::size_t, std::string > result;
    std::string line;
    while( std::getline( a_stream, line ) )
    {
        if( line.empty() )
            throw std::runtime_error{"Empty line in types file."};
        auto size = std::stoull( line.substr( 0, line.find( ' ' ) ) );
        auto name = line.substr( line.find( ' ' ) + 1 );
        result.emplace( size, name );
    }
    return result;
}

std::optional< std::string > get_string_at( score::binary_io::input_interface& a_input_interface, std::size_t a_offset, std::size_t a_length )
{
    auto is_valid_string = []( std::string_view a_string )
    {
        return !a_string.empty() &&
                std::ranges::all_of( a_string.substr( 0, a_string.size() ), 
                                    []( char a_c ) { return std::isprint( a_c ) || std::isspace( a_c ); } );
    };  
    // some strings are terminated with MULTIPLE null bytes
    // if the next struct needs to be aligned
    auto trim_null_bytes = []( std::string_view a_string )
    {
        auto pos = a_string.find_last_not_of( '\0' );
        return a_string.substr( 0, pos + 1 );
    };
    a_input_interface.seek( a_offset );
    std::string str = read_fixed_string( a_input_interface, a_length );
    if( str.ends_with( '\0' ) && is_valid_string( trim_null_bytes( str ) ) )
        return str.substr( 0, str.size() - 1 );
    return {};
}

int main( int argc, char** argv )
{
    auto options = cxxopts::Options{"section_analyze_subsections", "Try to guess the subsections present in a file."};
    options.add_options()
        ( "file", "The file to analyze", cxxopts::value< std::string >() )
        ( "types", "The types file (defaults to types.txt)", cxxopts::value< std::string >()->default_value( "types.txt" ) )
        ( "t,types-to-show", "The number of types to show", cxxopts::value< std::size_t >()->default_value( "1" ) );
    options.parse_positional( {"file"} );
    options.positional_help( "<file>" );
    auto result = options.parse( argc, argv );
    if( argc < 2 )
    {
        fmt::print( stderr, "{}\n", options.help() );
        return EXIT_FAILURE;
    }
    const auto types_file = result["types"].as< std::string >();
    // does the file exist
    auto types_map = [&types_file]
    {
        if( !std::filesystem::exists( types_file ) )
        {
            fmt::print( stderr, "Could not open '{}'; type information will not be available.\n", types_file );
            return std::multimap< std::size_t, std::string >{};
        }
        return load_types_map( std::ifstream{types_file} );
    }();
    const auto num_types_to_show = result["types-to-show"].as< std::size_t >();
    const auto filename = result["file"].as< std::string >();
    auto i_interface = score::binary_io::open_file( filename );
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
    {
        const auto num_matches = types_map.count( subsection.second );
        std::string type_suffix{};
        constexpr std::size_t k_min_size_to_show = 0x20;
        bool was_string = false;
        if( auto str = get_string_at( *i_interface, subsection.first + data_start, subsection.second ) )
        {
            type_suffix = fmt::format( " - probably string '{}'", *str );
            was_string = true;
        }
        else if( subsection.second > k_min_size_to_show && num_types_to_show > 0 )
        {
            if( num_matches == 0 )
                type_suffix = " - no matching types found";
            else if( num_matches == 1 )
                type_suffix = fmt::format( " - probably {}", types_map.find( subsection.second )->second );
            else if( num_matches > 1 )
                type_suffix = fmt::format( " - probably one of {} types", num_matches );
        }
        fmt::print( "sub {:6x} size {:4x}{}\n", subsection.first, subsection.second, type_suffix );
        if( subsection.second > k_min_size_to_show && num_matches > 1 && num_types_to_show > 1 && !was_string )
        {
            std::size_t num_shown = 0;
            auto range = types_map.equal_range( subsection.second );
            for( auto i = range.first; i != range.second && num_shown < num_types_to_show; ++i, ++num_shown )
                fmt::print( "  {}\n", i->second );
            if( std::distance( range.first, range.second ) > num_types_to_show )
                fmt::print( "  ...\n" );
        }
    }
    if( num_types_to_show > 0 )
    {
        fmt::print( "Up to {} types are shown, use --types-to-show 0 to disable\n", num_types_to_show );
        fmt::print( "Types shown are guesses based on the size of the subsection.\n" );
        fmt::print( "They are not guaranteed to be correct.\n" );
    }
    return EXIT_SUCCESS;
}
