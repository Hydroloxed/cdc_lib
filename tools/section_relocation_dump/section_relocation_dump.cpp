#include <algorithm>
#include <cdc_lib/resource/rsrc_relocation.h>
#include <cdc_lib/resource/rsrc_resource.h>
#include <cdc_lib/resource/tras/pc_w/tras_pc_w_relocation.h>
#include <cdc_lib/resource/tras/pc_w/tras_pc_w_resource.h>
#include <cxxopts.hpp>
#include <fmt/core.h>
#include <score/binary_io/binary_io.h>

namespace c_res = cdc_lib::resource;

void dump_external_relocation( const c_res::cooked_relocation& a_relocation )
{
    if( !a_relocation.resource->is_null_reference() )
    {
        auto unpacked = c_res::tras::pc_w::resource_ref_id{a_relocation.resource->get_user_id()};
        fmt::print( "external relocation to [ref to resource {}, type {}{}] @ {:#x}\n",
                    unpacked.resource_id,
                    unpacked.section_type,
                    a_relocation.resource->is_concrete_reference() ? " (concrete)" : "",
                    a_relocation.src_ptr_offset );
    }
    else
        fmt::print( "external relocation to [null ref] @ {:#x}\n", a_relocation.src_ptr_offset );
}

int main( int argc, char** argv )
{
    auto options = cxxopts::Options{"section_relocation_dump", ""};
    options.add_options()
        ( "file", "The section to dump", cxxopts::value< std::string >() );
    options.parse_positional( {"file"} );
    options.positional_help( "<file>" );
    auto result = options.parse( argc, argv );
    if( argc < 2 )
    {
        fmt::print( stderr, "{}\n", options.help() );
        return EXIT_FAILURE;
    }
    const auto filename = result["file"].as< std::string >();
    auto i_interface = score::binary_io::open_file( filename );
    auto relocations = c_res::tras::pc_w::load_relocation_table( *i_interface );
    std::ranges::sort( relocations, []( const auto& a, const auto& b )
                                    { return a.src_ptr_offset < b.src_ptr_offset; } );
    for( const auto& relocation : relocations )
    {
        if( relocation.is_internal() )
            fmt::print( "internal relocation to {:x} @ {:x}\n", relocation.dest_ptr_offset, relocation.src_ptr_offset );
        else
            dump_external_relocation( relocation );
    }
    return EXIT_SUCCESS;
}
