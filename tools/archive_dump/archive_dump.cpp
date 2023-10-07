#include "cdc_lib/file/archive_fs.h"
#include <cdc_lib/file/tras/pc_w/tras_pc_w_tiger.h>
#include <cstdio>
#include <cstdlib>
#include <cxxopts.hpp>
#include <fmt/core.h>
#include <fstream>

std::string guess_record_type( const cdc_lib::file::archive& a_archive, const cdc_lib::file::archive_record& a_record )
{
    const auto data = cdc_lib::file::tras::pc_w::read_record( a_archive, a_record );
    if( data.empty() )
        return "empty file";
    if( data[0] == '\x16' )
        return "drm";
    constexpr auto k_cine_magic_pos = 0x2010;
    if( data.size() >= (k_cine_magic_pos + sizeof( std::uint32_t )) &&
        data[k_cine_magic_pos] == 'E' &&
        data[k_cine_magic_pos + 1] == 'N' &&
        data[k_cine_magic_pos + 2] == 'I' &&
        data[k_cine_magic_pos + 3] == 'C' )
        return "cine";
    if( data.size() >= sizeof( std::uint32_t ) &&
        data[0] == '\x44' &&
        data[1] == '\xAC' )
        return "mul";
    return "unknown";
}

int main( int argc, char** argv )
{
    auto options = cxxopts::Options{"cdc_lib_archive_dump", "Dump target archive"};
    options.add_options()
        ( "file", "The archive to dump", cxxopts::value< std::string >() );
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
        fmt::print( stderr, "Could not open file '{}'\n", filename.c_str() );
        return EXIT_FAILURE;
    }
    stream.exceptions( std::ifstream::failbit | std::ifstream::badbit );
    auto i_interface = score::binary_io::create_input_interface( stream );
    auto archive = cdc_lib::file::tras::pc_w::load_archive( *i_interface, filename );
    fmt::print( "archive \"{}\":\n", filename );
    fmt::print( " count: {}\n", archive.archive_count );
    fmt::print( " dlc: {}\n", archive.dlc_index );
    fmt::print( " num records: {}\n", archive.records.size() );
    fmt::print( " config: {}\n", archive.config_name );
    for( const auto& record : archive.records )
    {
        const auto type = guess_record_type( archive, record );
        fmt::print( "record {:08x}, spec {:08x}, {:8} bytes, at {:08x}, type {}\n",
                    record.name_hash,
                    record.spec_mask,
                    record.size,
                    record.offset,
                    type );
    }
    return EXIT_SUCCESS;
}
