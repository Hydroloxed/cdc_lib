#include <cdc_lib/render/tras/pc_w/tras_pc_w_shader.h>
#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <cxxopts.hpp>
#include <fstream>
#include <score/binary_io/binary_io.h>

int main( int argc, char** argv )
{
    auto options = cxxopts::Options{"material_dump", "Print the contents of a material file to stdout."};
    options.add_options()
        ( "d,dump-files", "Dump the contents of individual shaders to the working directory" )
        ( "file", "The shader table to dump", cxxopts::value< std::string >() );
    options.parse_positional( {"file"} );
    auto result = options.parse( argc, argv );
    if( argc < 2 )
    {
        std::fprintf( stderr, "%s\n", options.help().c_str() );
        return EXIT_FAILURE;
    }
    const auto filename = result["file"].as< std::string >();
    auto stream = std::ifstream{filename};
    if( !stream.good() )
    {
        std::fprintf( stderr, "Could not open file '%s'\n", filename.c_str() );
        return EXIT_FAILURE;
    }
    auto i_interface = score::binary_io::create_input_interface( stream );
    auto shader_table = cdc_lib::render::tras::pc_w::load_shader_table( *i_interface );
    for( auto i = 0; const auto& shader : shader_table->shaders )
    {
        if( !shader.get() )
        {
            std::printf( "%d: <no shader>\n", i );
            i++;
            continue;
        }
        char buf[256]{};
        std::snprintf( buf, 256, "%016" PRIx64 ".shad", shader->id.lo );
        if( result["dump-files"].as< bool >() )
        {
            auto file = std::fopen( buf, "wb" );
            std::fwrite( shader->data.data(), shader->data.size(), 1, file );
            std::fclose( file );
        }
        std::printf( "%d: %016" PRIx64 " %08" PRIx32 " (%u bytes)\n",
                     i,
                     shader->id.lo,
                     shader->id.hi,
                     shader->id.size );
        i++;
    }
}
