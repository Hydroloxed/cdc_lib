#include <cdc_lib/render/tras/pc_w/tras_pc_w_material.h>
#include <cdc_lib/resource/rsrc_reloc_input_stream.h>
#include <cdc_lib/resource/tras/pc_w/tras_pc_w_relocation.h>
#include <cdc_lib/resource/tras/pc_w/tras_pc_w_resource.h>
#include <cstdio>
#include <fmt/core.h>
#include <fstream>
#include <score/binary_io/binary_io.h>
#include <string>

namespace c_res = cdc_lib::resource;
namespace c_ren = cdc_lib::render;

[[nodiscard]] auto indent( std::size_t a_level ) { return std::string( a_level * 2,' '); }

void dump_subsections( std::span< c_res::reloc_stream_subsection_info > a_subsections,
                       std::size_t a_indent_level = 0 )
{
    for( const auto& subsection : a_subsections )
        fmt::print( "{}subsection '{}' at {:#x}, read {:#x} bytes\n", indent( a_indent_level ), subsection.name, subsection.start_offset, subsection.bytes_read, subsection.bytes_read );
}

void dump_resource_ref( const char* a_label,
                        const c_res::resource_ref& a_ref,
                        std::size_t a_indent_level )
{
    if( !a_ref.is_null_reference() )
    {
        auto unpacked = c_res::tras::pc_w::resource_ref_id{ a_ref.get_user_id() };
        fmt::print( "{}{}: [ref to resource {}, type {}{}]\n", indent( a_indent_level ), a_label, unpacked.resource_id, unpacked.section_type, a_ref.is_concrete_reference() ? " (concrete)" : "" );
    }
    else
        fmt::print( "{}{}: [null ref]\n", indent( a_indent_level ), a_label );
}

void dump_constant_list( const char* a_label,
                         const cdc_lib::render::tras::pc_w::material_data::constant_list& a_constant_list,
                         std::size_t a_indent_level )
{
    if( a_constant_list.constants.empty() )
        return;
    fmt::print( "{}{}:\n", indent( a_indent_level ), a_label );
    for( std::size_t i = 0; i < a_constant_list.constants.size(); i++ )
    {
        const auto constant = a_constant_list.constants[i];
        if( a_constant_list.instance_param_count && i == a_constant_list.first_instance_param )
            fmt::print( "{}{} instance params {{\n", indent( a_indent_level + 1 ), a_constant_list.instance_param_count );
        else if( a_constant_list.extended_instance_param_count && i == a_constant_list.first_extended_instance_param )
            fmt::print( "{}{} extended instance params {{\n", indent( a_indent_level + 1 ), a_constant_list.extended_instance_param_count );
        else if( a_constant_list.instance_param_count && i == a_constant_list.first_instance_param + a_constant_list.instance_param_count )
            fmt::print( "{}}} // instance params\n", indent( a_indent_level + 1 ) );
        else if( a_constant_list.extended_instance_param_count && i == a_constant_list.first_extended_instance_param + a_constant_list.extended_instance_param_count )
            fmt::print( "{}}} // extended instance params\n", indent( a_indent_level + 1 ) );
        fmt::print( "{}cval {}: {}\n", indent( a_indent_level + 2 ), i, constant );
    }
}

void dump_texture_list( const char* a_label,
                        const cdc_lib::render::tras::pc_w::material_data::texture_list& a_tex_list,
                        std::size_t a_indent_level )
{
    if( a_tex_list.textures.empty() )
        return;
    fmt::print( "{}{}:\n", indent( a_indent_level ), a_label );
    for( std::size_t i = 0; i < a_tex_list.textures.size(); i++ )
    {
        const auto& entry = a_tex_list.textures[i];
        if( i == a_tex_list.first_instance_texture )
            fmt::print( "{}{} instance textures:\n", indent( a_indent_level ), a_tex_list.instance_texture_count );
        else if( i == a_tex_list.first_instance_texture + a_tex_list.instance_texture_count )
            fmt::print( "{}{} material textures:\n", indent( a_indent_level ), a_tex_list.material_texture_count );
        fmt::print( "{}texture {} cat '{}' type '{}' class '{}' slot '{}'",
                    indent( a_indent_level + 1 ),
                    i,
                    entry.category,
                    cdc_lib::render::tras::pc_w::material_data::texture_type_debugstr.lookup_or( entry.type, "INVALID" ),
                    cdc_lib::render::texture_class_debugstr.lookup_or( entry.class_, "INVALID" ),
                    entry.texture_slot
                  );
        dump_resource_ref( "", entry.texture, 0 );
    }
}

void dump_material( c_ren::tras::pc_w::material_data& a_material )
{
    using md = c_ren::tras::pc_w::material_data;
    fmt::print( "material dump:\n" );
    fmt::print( "version {:#x} id {} cwm {:#x} bm ??? bf {}\n",
                 a_material.version,
                 a_material.id,
                 a_material.color_write_mask,
                 a_material.blend_factor );
    fmt::print( "fog type '{}' fade mode '{}'\n", 
                md::fog_type_debugstr.lookup_or( a_material.fog_type_, "invalid" ),
                md::fade_mode_debugstr.lookup_or( a_material.fade_mode_, "invalid" ) );
    fmt::print( "dbgclr '{:08x}'\n", a_material.debug_color );
    fmt::print( "passes:\n" );
    for( auto i = 0; auto pass : a_material.passes )
    {
        i++;
        if( !pass.has_value() )
            continue;
        fmt::print( "{}{} pass ({}):\n",
                    indent( 1 ),
                    md::pass_index_debugstr.lookup_or( i - 1, "YOU SHOULD NOT SEE THIS!!!!!!" ),
                    i );
        dump_resource_ref( "ps", pass->pixel_shader, 2 );
        dump_resource_ref( "vs", pass->vertex_shader, 2 );
        dump_texture_list( "pixel textures", pass->pixel_textures, 2 );
        dump_constant_list( "pixel constants", pass->pixel_constants, 2 );
        dump_texture_list( "vertex textures", pass->vertex_textures, 2 );
        dump_constant_list( "vertex constants", pass->vertex_constants, 2 );
        if( !pass->hull_domain_shader_data.has_value() )
            continue;
        dump_resource_ref( "hs", pass->hull_domain_shader_data->hull_shader, 2 );
        dump_resource_ref( "ds", pass->hull_domain_shader_data->domain_shader, 2 );
        dump_texture_list( "hull textures", pass->hull_domain_shader_data->hull_textures, 2 );
        dump_constant_list( "hull constants", pass->hull_domain_shader_data->hull_constants, 2 );
        dump_texture_list( "domain textures", pass->hull_domain_shader_data->domain_textures, 2 );
        dump_constant_list( "domain constants", pass->hull_domain_shader_data->domain_constants, 2 );
        fmt::print( "{}fade start {:.3} range {:.3}\n",
                    indent( 2 ),
                    pass->hull_domain_shader_data->fade_start,
                    pass->hull_domain_shader_data->fade_range );
    }
}

int main( int argc, char** argv )
{
    if( argc < 2 )
        return EXIT_FAILURE;

    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    auto stream = std::ifstream{ argv[1], std::ifstream::in | std::ifstream::binary };
    stream.exceptions( std::ifstream::failbit | std::ifstream::badbit | std::ifstream::eofbit );
    auto interface = score::binary_io::create_input_interface( stream );
    auto relocations = c_res::tras::pc_w::load_relocation_table( *interface );
    auto reloc_istream = c_res::reloc_istream{ *interface, relocations, sizeof( std::uint32_t ) };
    try
    {
        auto material = c_ren::tras::pc_w::load_material( reloc_istream );
        dump_material( *material );
    }
    catch( const std::exception& ex )
    {
        fmt::print( "An error was thrown while loading the material:\n" );
        fmt::print( "  {}\n", typeid(ex).name() );
        fmt::print( "  what(): {}\n", ex.what() );
        fmt::print( "Here is a dump of all subsections in the file:\n" );
        dump_subsections( reloc_istream.get_subsections() );
    }
    catch( ... )
    {
        fmt::print( "An unknown error was thrown while loading the material.\n" );
        fmt::print( "Here is a dump of all subsections in the file:\n" );
        dump_subsections( reloc_istream.get_subsections() );
    }
}
