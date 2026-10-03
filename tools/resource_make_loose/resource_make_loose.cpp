#include <cdc_lib/core/core_crc32.h>
#include <cdc_lib/file/tras/pc_w/tras_pc_w_compression.h>
#include <cdc_lib/file/archive_fs.h>
#include <cdc_lib/resource/tras/pc_w/tras_pc_w_resolve_object.h>
#include <cstdlib>
#include <cxxopts.hpp>
#include <fmt/base.h>
#include <fstream>

int main( int argc, char** argv )
{
    auto options = cxxopts::Options{"cdc_lib_resource_make_loose", "Make a loose .DRM"};
    options.add_options()
        ( "gamedir", "The game directory", cxxopts::value< std::string >() )
        ( "drmname", "The DRM name", cxxopts::value< std::string >() )
        ( "outdir", "The output directory", cxxopts::value< std::string >() );
    options.parse_positional( {"gamedir", "outdir", "drmname"} );
    options.positional_help( "<gamedir> <out directory> <drmname>" );
    auto result = options.parse( argc, argv );
    if( argc < 4 || result.contains( "help" ) )
    {
        fmt::print( stderr, "{}\n", options.help() );
        return EXIT_FAILURE;
    }
    auto multifs = cdc_lib::file::make_multifs_tras( result["gamedir"].as< std::string >() );
    auto* bigfile = &multifs.archives[0];
    cdc_lib::file::archive_record* record = nullptr;
    std::string drm_path = bigfile->config_name + "\\" + result["drmname"].as< std::string >();
    const auto path_hash = cdc_lib::core::crc32( drm_path );
    for( auto it = multifs.archives.rbegin(); it != multifs.archives.rend(); ++it )
    {
        auto& records = it->second.records;
        const auto found = std::ranges::find_if( records.begin(), records.end(), [path_hash]( const auto& a_record ) { return a_record.name_hash == path_hash; } );
        if( found != records.end() )
        {
            bigfile = &it->second;
            record = &*found;
            break;
        }
    }
    // const auto record = std::ranges::find_if( bigfile->records, [path_hash]( const auto& a_record ) { return a_record.name_hash == path_hash; } );
    if( !record )
    {
        fmt::print( stderr, "'{}' not found in any archive\n", drm_path );
        return EXIT_FAILURE;
    }
    const auto record_data = cdc_lib::file::read_record( *bigfile, *record );
    auto i_interface = score::binary_io::create_input_interface( record_data );
    const auto resolve_object = cdc_lib::resource::tras::pc_w::load_object( *i_interface );
    std::vector< std::string > section_datas;
    for( const auto& section : resolve_object->sections )
    {
        std::string data = cdc_lib::file::read_offset( multifs, section.extra_data.packed_offset, section.extra_data.compressed_size );
        auto compressed_interface = score::binary_io::create_input_interface( data );
        auto decompressed_data = cdc_lib::file::tras::pc_w::decompress_cdrm( *compressed_interface );
        section_datas.push_back( decompressed_data );
    }
    const auto out_path = result["outdir"].as< std::string >() + "\\cooked_" + bigfile->config_name + "\\" + result["drmname"].as< std::string >();
    std::filesystem::create_directories( out_path.substr( 0, out_path.rfind( "\\" ) ) );
    std::ofstream out_stream{out_path, std::ios::out | std::ios::binary};
    if( !out_stream.good() )
    {
        fmt::print( stderr, "Could not open output file ;{};\n", out_path );
        return EXIT_FAILURE;
    }
    out_stream.exceptions( std::ios::badbit | std::ios::eofbit | std::ios::failbit );
    auto o_interface = score::binary_io::create_output_interface( out_stream );
    cdc_lib::resource::tras::pc_w::write_object( *o_interface, *resolve_object, section_datas );
    return EXIT_SUCCESS;
}
