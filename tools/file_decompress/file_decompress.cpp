#include <cdc_lib/file/tras/pc_w/tras_pc_w_compression.h>
#include <cxxopts.hpp>
#include <fmt/core.h>
#include <fstream>

int main( int argc, char** argv )
{
    auto options = cxxopts::Options{"resource_decompress", "Decompress CDRM resource"};
    options.add_options()
        ( "i,input", "Input CDRM file", cxxopts::value< std::string >() )
        ( "o,output", "Output file", cxxopts::value< std::string >() )
        ( "h,help", "Print help" );
    options.parse_positional( {"input"} );
    options.positional_help( "<input>" );

    auto result = options.parse( argc, argv );
    if( argc < 2 || result.count("help") != 0 )
    {
        fmt::print( "{}\n", options.help() );
        return EXIT_FAILURE;
    }
    const auto filename = result["input"].as< std::string >();
    std::ifstream stream{filename, std::ios::binary};
    if( !stream.good() )
    {
        fmt::print(stderr, "Could not open file '{}'\n", filename.c_str());
        return EXIT_FAILURE;
    }
    stream.exceptions( std::ios::badbit | std::ios::eofbit | std::ios::failbit );
    auto input_interface = score::binary_io::create_input_interface( stream, std::endian::little );
    auto decompressed = cdc_lib::file::tras::pc_w::decompress_cdrm( *input_interface );
    const auto output_filename = [&]
    {
        if( result.count("output") == 0 )
            return filename + "_decompressed.bin";
        return result["output"].as< std::string >();
    }();
    std::ofstream output{output_filename, std::ios::binary};
    output.exceptions( std::ios::badbit | std::ios::eofbit | std::ios::failbit );
    output.write( decompressed.data(), static_cast< std::streamsize >( decompressed.size() ) );
    return EXIT_SUCCESS;
}
