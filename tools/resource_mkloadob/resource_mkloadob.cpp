#include <cdc_lib/resource/rsrc_relocation.h>
#include <cdc_lib/resource/tras/pc_w/tras_pc_w_relocation.h>
#include <cdc_lib/resource/tras/pc_w/tras_pc_w_resource.h>
#include <cxxopts.hpp>
#include <fmt/core.h>
#include <fstream>
#include <optional>
#include <score/binary_io/binary_io.h>
#include <string>

namespace
{
    void trim_tailing_whitespace( std::string& a_string )
    {
        while( !a_string.empty() && std::isspace( static_cast< unsigned char >( a_string.back() ) ) )
            a_string.pop_back();
    }

    void trim_leading_whitespace( std::string& a_string )
    {
        while( !a_string.empty() && std::isspace( static_cast< unsigned char >( a_string.front() ) ) )
            a_string.erase( 0, 1 );
    }

    std::vector< std::string > preprocess( const std::string& a_source )
    {
        std::vector< std::string > ret{};
        std::istringstream iss{a_source};
        for( std::string l; std::getline( iss, l ); )
        {
            if( auto comment = l.find( "//" ); comment != std::string::npos )
                l = l.substr( 0, comment );
            if( auto comment = l.find( ';' ); comment != std::string::npos )
                l = l.substr( 0, comment );
            trim_tailing_whitespace( l );
            trim_leading_whitespace( l );
            if( l.empty() )
                continue;
            ret.push_back( l );
        }
        return ret;
    }

    struct loadob_internal_relocation
    {
        std::uint32_t src_offset;
        std::string referenced_subsection{};
    };

    struct loadob_file
    {
        std::string binary_data{};
        std::map< std::string, std::size_t > subsection_offsets{};
        std::vector< loadob_internal_relocation > internal_relocations{};
        std::vector< cdc_lib::resource::cooked_relocation > relocations{};
    };

    std::optional< loadob_file > load( const std::string& a_data )
    {
        auto lines = preprocess( a_data );
        loadob_file ret{};
        auto output = score::binary_io::create_output_interface( ret.binary_data );
        for( const auto& line : lines )
        {
            if( line.empty() )
                continue;
            
            const auto err_if_not_aligned = [&output, &line]( std::size_t a_alignment )
            {
                if( output->tell() % a_alignment != 0 )
                {
                    fmt::print( stderr, "{}\n", line );
                    fmt::print( stderr, "ERROR: not aligned to {} bytes!\n", a_alignment );
                }
            };

            // subsection
            // for example: "[My level]"
            if( line.starts_with( '[' ) )
            {
                const auto name = line.substr( 1, line.rfind( ']' ) - 1 );

                // 'ptr=mysubsection  ' will be truncated to 'ptr=mysubsection' during preprocessing
                if( std::isspace( static_cast< unsigned char >( name.back() ) ) )
                    fmt::print( stderr, "WARNING: subsection '{}' contains trailing whitespace."
                                        "It won't be referenceable via 'ptr='!\n", name );
                ret.subsection_offsets[name] = output->tell();
                fmt::print( "subsection '{}' @ {:3x}\n", name, output->tell() );
            }
            else if( line.starts_with( "align=" ) )
            {
                const auto alignment = std::stoul( line.substr( sizeof "align=" - 1 ) );
                while( output->tell() % alignment != 0 )
                    write< std::uint8_t >( *output, 0 );
            }
            else if( line.starts_with( "ptr=" ) )
            {
                err_if_not_aligned( 4 );
                const auto subsection_name = line.substr( sizeof "ptr=" - 1 );
                const bool is_null = subsection_name == "null" || subsection_name == "0";
                if( !is_null )
                {
                    ret.internal_relocations.push_back( {.src_offset = static_cast< std::uint32_t >( output->tell() ),
                                                         .referenced_subsection = subsection_name} );
                }
                constexpr std::uint32_t k_pointer_placeholder = 0xbeebbeebul;
                write< std::uint32_t >( *output, is_null ? 0ul : k_pointer_placeholder ); // TODO: 32-bit only
            }
            else if( line.starts_with( "externptr=" ) )
            {
                err_if_not_aligned( 4 );
                const auto data = line.substr( sizeof "externptr=" - 1 );
                std::array< std::string::size_type, 3 > colon_pos{};
                colon_pos[0] = data.find( ':' );
                colon_pos[1] = data.find( ':', colon_pos[0] + 1 );
                if( colon_pos[0] == std::string::npos )
                {
                    fmt::print( stderr, "{}\n", line );
                    fmt::print( stderr, "ERROR: invalid externptr format!\n" );
                    fmt::print( stderr, "Wanted format: externptr=:<type>:<id>[:offset]\n" );
                    return std::nullopt;
                }
                const auto type = data.substr( 0, colon_pos[0] );
                std::uint8_t type_int = std::stoul( type, nullptr, 0 );
                const auto id = data.substr( colon_pos[0] + 1,
                                             colon_pos[1] == std::string::npos
                                               ? std::string::npos
                                               : colon_pos[1] - colon_pos[0] - 1 );
                std::uint32_t id_int = std::stoul( id, nullptr, 0 );
                auto offset = 0ul;
                if( colon_pos[1] != std::string::npos )
                    offset = std::stoul( data.substr( colon_pos[1] + 1 ), nullptr, 0 );

                using namespace cdc_lib::resource;

                tras::pc_w::resource_ref_id ref_id{id_int, type_int};
                cooked_relocation relocation = {};
                relocation.resource = resource_ref{ref_id.pack()};
                relocation.src_ptr_offset = static_cast< std::uint32_t >( output->tell() );
                relocation.dest_ptr_offset = offset;
                relocation.type = cooked_relocation_type::external;
                ret.relocations.push_back( relocation );
                // We can just write whatever here, the actual value will be filled in later
                write< std::uint32_t >( *output, 0x0 );
            }
            else if( line.starts_with( "uint" ) || line.starts_with( "int" ) )
            {
                const auto int_str = line.substr( line.find( "=" ) + 1 );
                // use base 0, so that 0xabc is correctly parsed as a hex number
                const std::intmax_t int_data = std::stoll( int_str, nullptr, 0 );

                auto maybe_write_integer = [&]< typename signed_type, typename unsigned_type >
                                        ( std::string_view a_signed_name, std::string_view a_unsigned_name )
                {
                    if( line.starts_with( a_signed_name ) )
                        write< signed_type >( *output, static_cast< signed_type >( int_data ) );
                    else if( line.starts_with( a_unsigned_name ) )
                        write< unsigned_type >( *output, static_cast< unsigned_type >( int_data ) );
                };
                // yes, this IS the syntax for calling a templated lambda with explicit template arguments...
                maybe_write_integer.operator()< std::int8_t, std::uint8_t >( "int8", "uint8" );
                maybe_write_integer.operator()< std::int16_t, std::uint16_t >( "int16", "uint16" );
                maybe_write_integer.operator()< std::int32_t, std::uint32_t >( "int32", "uint32" );
                maybe_write_integer.operator()< std::int64_t, std::uint64_t >( "int64", "uint64" );
            }
            else if( line.starts_with( "float32=" ) )
            {
                const auto float_str = line.substr( line.find( "=" ) + 1 );
                const float float_data = std::stof( float_str );
                write< float >( *output, float_data );
            }
            else if( line.starts_with( "pad=" ) )
            {
                const auto pad_str = line.substr( line.find( "=" ) + 1 );
                const auto pad_bytes = std::stoll( pad_str );
                for( std::size_t i = 0; i < pad_bytes; i++ )
                    write< std::uint8_t >( *output, 0 );
            }
            else if( line.starts_with( "str=") )
            {
                auto str = line.substr( line.find( "=" ) + 1 );
                if( str.back() == '\"' )
                    str.pop_back();
                for( const auto ch : str )
                    write< std::uint8_t >( *output, static_cast< std::uint8_t >( ch ) );
                write< std::uint8_t >( *output, 0 );
            }
            else
            {
                fmt::print( "{}", line );
                fmt::print( stderr, "ERROR: unrecognized line '{}'\n", line );
                return std::nullopt;
            }
        }
        return ret;
    }

    void write_with_relocations( loadob_file& a_file, score::binary_io::output_interface& a_output )
    {
        std::vector< cdc_lib::resource::cooked_relocation > relocations = a_file.relocations;
        for( const auto& relocation : a_file.internal_relocations )
        {
            if( !a_file.subsection_offsets.contains( relocation.referenced_subsection ) )
            {
                fmt::print( stderr, "ERROR: could not find subsection '{}'!\n", relocation.referenced_subsection );
            }
            else
            {
                const auto dest_offset = static_cast< std::uint32_t >( a_file.subsection_offsets.at( relocation.referenced_subsection ) );
                relocations.push_back( {.src_ptr_offset = relocation.src_offset, .dest_ptr_offset = dest_offset} );
            }
        }
        cdc_lib::resource::tras::pc_w::write_relocation_table( a_output, relocations, a_file.binary_data );
        a_output.write( std::as_bytes( std::span{a_file.binary_data.data(), a_file.binary_data.size()} ) );
    }
}

int main( int argc, char** argv )
try
{
    auto options = cxxopts::Options{"resource_mkloadob", "Create a binary resource from a text file"};
    options.add_options()
        ( "i,input", "Input text file", cxxopts::value< std::string >() )
        ( "o,output", "Output binary file", cxxopts::value< std::string >() )
        ( "h,help", "Print help" );
    options.parse_positional( {"input", "output"} );
    options.positional_help( "<input> <output>" );
    auto result = options.parse( argc, argv );
    if( argc < 3 || result.contains( "help" ) )
    {
        fmt::print( "{}\n", options.help() );
        return EXIT_FAILURE;
    }
    const auto input_filename = result["input"].as< std::string >();
    std::ifstream input_file{input_filename};
    // input_file.exceptions( std::ios::badbit | std::ios::eofbit | std::ios::failbit );
    const auto input_size = std::filesystem::file_size( input_filename );
    std::string input_data( input_size, '\0'  );
    input_file.read( input_data.data(), static_cast< std::streamsize >( input_size ) );

    if( auto loadob_f = load( input_data ) )
    {
        std::ofstream output_file{result["output"].as< std::string >()};
        const auto output = score::binary_io::create_output_interface( output_file );
        write_with_relocations( *loadob_f, *output );
    }
}
catch( std::exception& e )
{
    fmt::print( stderr, "UNKNOWN ERROR: {}\n", e.what() );
    return EXIT_FAILURE;
}
