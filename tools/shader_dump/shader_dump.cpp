#include <cdc_lib/render/tras/pc_w/tras_pc_w_shader.h>
#include <cinttypes>
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <score/binary_io/binary_io.h>

int main( int argc, char** argv )
{
    if( argc < 2 )
        std::exit( EXIT_FAILURE );
    auto stream = std::ifstream{ argv[1] };
    if( !stream.good() )
        std::exit( EXIT_FAILURE );
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
        auto file = std::fopen( buf, "wb" );
        std::fwrite( shader->data.data(), shader->data.size(), 1, file );
        std::fclose( file );
        std::printf( "%d: %016" PRIx64 " %08" PRIx32 " (%u bytes)\n",
                     i,
                     shader->id.lo,
                     shader->id.hi,
                     shader->id.size );
        i++;
    }
}
