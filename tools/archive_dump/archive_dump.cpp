#include <cdc_lib/file/tras/pc_w/tras_pc_w_tiger.h>
#include <cstdio>
#include <cstdlib>
#include <cxxopts.hpp>
#include <fmt/core.h>
#include <fstream>


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
        fmt::print( "record {:08x}, spec {:08x}, {:8} bytes, at {:08x}\n", record.name_hash, record.spec_mask, record.size, record.offset );
    }
    return EXIT_SUCCESS;
}
