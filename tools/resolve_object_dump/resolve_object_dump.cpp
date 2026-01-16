#include <cdc_lib/resource/rsrc_resolve_object.h>
#include <cdc_lib/resource/tras/pc_w/tras_pc_w_resolve_object.h>
#include <cxxopts.hpp>
#include <fmt/color.h>
#include <fmt/core.h>
#include <score/binary_io/binary_io.h>
#include <score/containers/simple_lookup_table.h>

const char* get_section_short_name( cdc_lib::resource::cooked_resolve_section_type a_section_type )
{
    using enum cdc_lib::resource::cooked_resolve_section_type;
    switch( a_section_type )
    {
        case unknown: return "unkn";
        case general: return "gnrl";
        case animation: return "anim";
        case texture: return "tex";
        case wave: return "wave";
        case dtp: return "dtp";
        case script: return "scrt";
        case shader: return "shdr";
        case material: return "matd";
        case object: return "objt";
        case render_resource: return "rndr";
        case collision_mesh: return "cmsh";
    }
    return "unkn";
}

void dump_section( const cdc_lib::resource::cooked_resolve_section& a_section,
                   bool a_verbose,
                   int  a_index )
{
    if( !a_verbose )
        fmt::print( "section {:4}, id {:6}, size '{:6}', type '{:4}', rt '{}', offset '{:08x}, dcmpo '{:08x}'\n",
                    a_index,
                    a_section.id,
                    a_section.size,
                    get_section_short_name( a_section.type ),
                    // Display non-zero resource types in noticeable colors.
                    // TODO: make this configurable
                    fmt::styled( fmt::format( "{:2x}", a_section.resource_type ),
                                           fmt::fg( a_section.resource_type == 0 ? fmt::color::dark_gray : fmt::color::white ) ),
                    a_section.extra_data.packed_offset,
                    a_section.extra_data.decompressed_offset );
}

int main( int argc, char** argv )
{
    auto options = cxxopts::Options{"resolve_object_dump ", "Print the contents of a material file to stdout."};
    options.add_options()
        ( "v,verbose", "Print verbose section info" )
        ( "t,filter-type", "Only show sections of a certain type", cxxopts::value< std::uint32_t >() )
        ( "file", "The resolve object to dump", cxxopts::value< std::string >() );
    options.parse_positional( {"file"} );
    auto result = options.parse( argc, argv );
    if( argc < 2 )
    {
        fmt::print( stderr, "{}\n", options.help() );
        return EXIT_FAILURE;
    }
    const auto filename = result["file"].as< std::string >();
    auto i_interface = score::binary_io::open_file( filename );
    auto resolve_object = cdc_lib::resource::tras::pc_w::load_object( *i_interface );
    if( !resolve_object->includes.empty() )
    {
        fmt::print( "includes objects:\n" );
        for( const auto& include : resolve_object->includes )
            fmt::print( "    {}\n", include );
    }
    if( !resolve_object->objects_that_depend_on.empty() )
    {
        fmt::print( "required by objects:\n" );
        for( const auto& dependency : resolve_object->objects_that_depend_on )
            fmt::print( "    {}\n", dependency );
    }
    auto filter_pass = [&]( const cdc_lib::resource::cooked_resolve_section& a_section )
    {
        bool passed = true;
        if( result.count( "filter-type") != 0u )
        {
            const auto filter_type = static_cast< cdc_lib::resource::cooked_resolve_section_type >
                                       ( result["filter-type"].as< std::uint32_t >() );
            passed = passed && a_section.type == filter_type;
        }
        return passed;
    };
    for( int i = 0; const auto& section : resolve_object->sections )
    {
        if( filter_pass( section ) )
            dump_section( section, result["verbose"].as< bool >(), i + 1 );
        i++;
    }
    return EXIT_SUCCESS;
}
