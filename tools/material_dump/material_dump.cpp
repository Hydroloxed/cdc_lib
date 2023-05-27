#include <cdc_lib/render/tras/pc_w/tras_pc_w_material.h>
#include <cdc_lib/resource/tras/pc_w/tras_pc_w_relocation.h>
#include <cdc_lib/resource/tras/pc_w/tras_pc_w_resource.h>
#include <cdc_lib/resource/rsrc_reloc_input_stream.h>
#include <cstdio>
#include <fstream>
#include <score/binary_io/binary_io.h>

namespace c_res = cdc_lib::resource;
namespace c_ren = cdc_lib::render;

void dump_resource_ref( const char* a_label, const c_res::resource_ref& a_ref )
{
    if( !a_ref.is_null_reference() )
    {
        auto unpacked = c_res::tras::pc_w::resource_ref_id{ a_ref.get_user_id() };
        std::printf( "%s: [ref to resource %u, type %u%s]\n",
                    a_label,
                    unpacked.resource_id,
                    unpacked.section_type,
                    a_ref.is_concrete_reference() ? " (concrete)" : "" );
    }
    else
        std::printf( "%s: [null ref]\n", a_label );
}

void dump_constant_list( const char* a_label, const cdc_lib::render::tras::pc_w::material_data::constant_list& a_constant_list )
{
    if( a_constant_list.constants.empty() )
        return;
    std::printf( "%s:\n", a_label );
    for( std::size_t i = 0; i < a_constant_list.constants.size(); i++ )
    {
        const auto constant = a_constant_list.constants[i];
        if( a_constant_list.instance_param_count && i == a_constant_list.first_instance_param )
            std::printf( "      %u instance params {\n", a_constant_list.instance_param_count );
        else if( a_constant_list.extended_instance_param_count && i == a_constant_list.first_extended_instance_param )
            std::printf( "      %u extended instance params {\n", a_constant_list.extended_instance_param_count );
        else if( a_constant_list.instance_param_count && i == a_constant_list.first_instance_param + a_constant_list.instance_param_count )
            std::printf( "      } // instance params\n" );
        else if( a_constant_list.extended_instance_param_count && i == a_constant_list.first_extended_instance_param + a_constant_list.extended_instance_param_count )
            std::printf( "      } // extended instance params\n" );
        std::printf( "      cval %lu: %.3f\n", i, constant );
    }
}

void dump_texture_list( const char* a_label, const cdc_lib::render::tras::pc_w::material_data::texture_list& a_tex_list )
{
    if( a_tex_list.textures.empty() )
        return;
    std::printf( "%s:\n", a_label );
    for( std::size_t i = 0; i < a_tex_list.textures.size(); i++ )
    {
        const auto& entry = a_tex_list.textures[i];
        if( i == a_tex_list.first_instance_texture )
            std::printf( "      %u instance textures:\n", a_tex_list.instance_texture_count );
        else if( i == a_tex_list.first_instance_texture + a_tex_list.instance_texture_count )
            std::printf( "      %u material textures:\n", a_tex_list.material_texture_count );
        std::printf( "      texture %lu cat '%u' type '%s' class '%s' slot '%u'",
                     i,
                     entry.category,
                     cdc_lib::render::tras::pc_w::material_data::texture_type_debugstr.lookup_or( entry.type, "INVALID" ),
                     cdc_lib::render::texture_class_debugstr.lookup_or( entry.class_, "INVALID" ),
                     entry.texture_slot
                   );
        dump_resource_ref( "", entry.texture );
    }
}

void dump_material( c_ren::tras::pc_w::material_data& a_material )
{
    using md = c_ren::tras::pc_w::material_data;
    std::printf( "material dump:\n" );
    std::printf( "version %x id %u cwm %x bm ??? bf %u\n",
                 a_material.version,
                 a_material.id,
                 a_material.color_write_mask,
                 a_material.blend_factor );
    std::printf( "fog type '%s' fade mode '%s'\n",
                 md::fog_type_debugstr.lookup_or( a_material.fog_type_, "invalid" ),
                 md::fade_mode_debugstr.lookup_or( a_material.fade_mode_, "invalid" ) );
    std::printf( "dbgclr '%08x'\n", a_material.debug_color );
    std::printf( "passes:\n" );
    for( auto i = 0; auto pass : a_material.passes )
    {
        i++;
        if( !pass.has_value() )
            continue;
        std::printf( "  %s pass (%d):\n",
                    cdc_lib::render::tras::pc_w::material_data::pass_index_debugstr.lookup_or( i - 1, "YOU SHOULD NOT SEE THIS!!!!!!" ),
                    i - 1 );
        dump_resource_ref( "    ps", pass->pixel_shader );
        dump_resource_ref( "    vs", pass->vertex_shader );
        dump_texture_list( "    pixel textures", pass->pixel_textures );
        dump_constant_list( "    pixel constants", pass->pixel_constants );
        dump_texture_list( "    vertex textures", pass->vertex_textures );
        dump_constant_list( "    vertex constants", pass->vertex_constants );
        if( !pass->hull_domain_shader_data.has_value() )
            continue;
        dump_resource_ref( "    hs", pass->hull_domain_shader_data->hull_shader );
        dump_resource_ref( "    ds", pass->hull_domain_shader_data->domain_shader );
        dump_texture_list( "    hull textures", pass->hull_domain_shader_data->hull_textures );
        dump_constant_list( "    hull constants", pass->hull_domain_shader_data->hull_constants );
        dump_texture_list( "    domain textures", pass->hull_domain_shader_data->domain_textures );
        dump_constant_list( "    domain constants", pass->hull_domain_shader_data->domain_constants );
        std::printf( "    fade start %.3f range %.3f\n",
                     pass->hull_domain_shader_data->fade_start,
                     pass->hull_domain_shader_data->fade_range );
    }
}

int main( int argc, char** argv )
{
    if( argc < 2 )
        return EXIT_FAILURE;
    auto stream = std::ifstream{ argv[1] };
    stream.exceptions( std::ifstream::failbit );
    auto interface = score::binary_io::create_input_interface( stream );
    auto relocations = c_res::tras::pc_w::load_relocation_table( *interface );
    auto reloc_istream = c_res::reloc_istream{ *interface, relocations, sizeof( std::uint32_t ) };
    auto material = c_ren::tras::pc_w::load_material( reloc_istream );
    dump_material( *material );
}
