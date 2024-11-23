#include <cdc_lib/render/tras/pc_w/tras_pc_w_shader.h>
#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <cxxopts.hpp>
#include <fmt/core.h>
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
        fmt::print( stderr, "{}\n", options.help() );
        return EXIT_FAILURE;
    }
    const auto filename = result["file"].as< std::string >();
    auto i_interface = score::binary_io::open_file( filename );
    auto shader_table = cdc_lib::render::tras::pc_w::load_shader_table( *i_interface );
    for( auto i = 0; const auto& shader : shader_table->shaders )
    {
        if( !shader.get() )
        {
            fmt::print( "{:2}: <no shader>\n", i );
            i++;
            continue;
        }
        if( result["dump-files"].as< bool >() )
        {
            const auto filename = fmt::format( "{:16x}.shad", shader->id.lo );
            auto file = std::ofstream{filename};
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
            file.write( reinterpret_cast< char* >( shader->data.data() ), static_cast< std::streamsize >( shader->data.size() ) );
        }
        fmt::print( "{:2}: {:016x} {:08x} (size: {} bytes)\n", i, shader->id.lo, shader->id.hi, shader->id.size );
        i++;
    }
}
