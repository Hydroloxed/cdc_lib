#include "tras_pc_w_material.h"
#include "score/score_bit.h"
#include <cassert>
#include <cstdint>
#include <score/containers/simple_lookup_table.h>

namespace cdc_lib::render::tras::pc_w
{
    namespace
    {
        constexpr score::simple_lookup_table< std::uint32_t, material_data::fog_type, 4 > k_fog_type_lut =
        {
            std::pair{ 0u, material_data::fog_type::none },
            std::pair{ 1u, material_data::fog_type::linear_dist },
            std::pair{ 2u, material_data::fog_type::non_linear_dist },
            std::pair{ 3u, material_data::fog_type::non_linear_dist_and_height },
        };

        void read( score::binary_io::input_interface& a_interface, material_data::fog_type& a_ft )
        {
            a_ft = k_fog_type_lut.lookup_or( read< std::uint16_t >( a_interface ), material_data::fog_type::invalid );
        }

        constexpr score::simple_lookup_table< std::uint32_t, material_data::fade_mode, 6 > k_fade_mode_lut =
        {
            std::pair{ 0u, material_data::fade_mode::off },
            std::pair{ 1u, material_data::fade_mode::alpha_blend },
            std::pair{ 2u, material_data::fade_mode::alpha_test },
            std::pair{ 3u, material_data::fade_mode::alpha_to_mask },
            std::pair{ 4u, material_data::fade_mode::alpha_to_mask_dither },
            std::pair{ 5u, material_data::fade_mode::clip_or_blend }
        };

        void read( score::binary_io::input_interface& a_interface, material_data::fade_mode& a_fm )
        {
            a_fm = k_fade_mode_lut.lookup_or( read< std::uint16_t >( a_interface ), material_data::fade_mode::invalid );
        }

        constexpr score::simple_lookup_table< std::uint32_t, texture_filter, 10 > k_texture_filter_lut =
        {
            std::pair{0u, texture_filter::point},
            std::pair{1u, texture_filter::bilinear},
            std::pair{2u, texture_filter::trilinear},
            std::pair{3u, texture_filter::anisotropic_1x},
            std::pair{4u, texture_filter::anisotropic_2x},
            std::pair{5u, texture_filter::anisotropic_4x},
            std::pair{6u, texture_filter::anisotropic_8x},
            std::pair{7u, texture_filter::anisotropic_16x},
            std::pair{8u, texture_filter::best},
            std::pair{9u, texture_filter::default_}
        };

        void read( score::binary_io::input_interface& a_interface, texture_filter& a_filter )
        {
            a_filter = k_texture_filter_lut.lookup_or( read< std::uint16_t >( a_interface ), texture_filter::invalid );
        }

        constexpr std::uint32_t k_version = 0x13;
        constexpr std::uint32_t k_magic = 0x4d617444;

        constexpr std::uint32_t k_resource_name_size = 256;

        [[nodiscard]] constexpr bool check_flag( std::uint32_t a_flags, std::uint32_t a_flag ) noexcept
        {
            return (a_flags & a_flag) != 0u;
        }

        namespace flags
        {
            constexpr std::uint32_t k_highlight = 0x1;
            constexpr std::uint32_t k_alpha_highlight = 0x2;
            constexpr std::uint32_t k_fog = 0x4;
            constexpr std::uint32_t k_affects_position = 0x8;
            constexpr std::uint32_t k_double_sided = 0x80;
            constexpr std::uint32_t k_2d = 0x200;
            constexpr std::uint32_t k_translucent_depth_write = 0x400;
            constexpr std::uint32_t k_back_before_front = 0x800;
            constexpr std::uint32_t k_sunlight_enabled = 0x1000;
            constexpr std::uint32_t k_draw_back_face_only = 0x2000;
            constexpr std::uint32_t k_pre_translucent_render = 0x4000;
            constexpr std::uint32_t k_tessellation = 0x8000;

            constexpr std::uint32_t k_combined_mask = k_highlight | k_alpha_highlight | k_fog | k_affects_position | k_double_sided | k_2d | k_translucent_depth_write | k_back_before_front | k_sunlight_enabled | k_draw_back_face_only | k_pre_translucent_render | k_tessellation;
        }
        void read( score::binary_io::input_interface& a_interface, material_data::flags& a_flags )
        {
            const auto flags = read< std::uint32_t >( a_interface );
            assert( (flags & ~flags::k_combined_mask) == 0 );
            a_flags.highlight = check_flag( flags, flags::k_highlight );
            a_flags.alpha_highlight = check_flag( flags, flags::k_alpha_highlight );
            a_flags.fog = check_flag( flags, flags::k_fog );
            a_flags.affects_position = check_flag( flags, flags::k_affects_position );
            a_flags.double_sided = check_flag( flags, flags::k_double_sided );
            a_flags._2d = check_flag( flags, flags::k_2d );
            a_flags.translucent_depth_write = check_flag( flags, flags::k_translucent_depth_write );
            a_flags.back_before_front = check_flag( flags, flags::k_back_before_front );
            a_flags.sunlight_enabled = check_flag( flags, flags::k_sunlight_enabled );
            a_flags.draw_back_face_only = check_flag( flags, flags::k_draw_back_face_only );
            a_flags.pre_translucent_render = check_flag( flags, flags::k_pre_translucent_render );
            a_flags.tessellation = check_flag( flags, flags::k_tessellation );
        }

        namespace node_flags
        {
            constexpr std::uint32_t k_irradiance_lighting = 0x1;
            constexpr std::uint32_t k_predator = 0x2;
            constexpr std::uint32_t k_local_reflection_map = 0x4;
            constexpr std::uint32_t k_back_buffer_resolve = 0x8;
            constexpr std::uint32_t k_use_light_map = 0x10;
            constexpr std::uint32_t k_no_deferred_specular = 0x20;
            constexpr std::uint32_t k_no_normal_write = 0x40;
            constexpr std::uint32_t k_depth_occluder = 0x80;
            // No, I don't know what happened to these missing flags.
            constexpr std::uint32_t k_stipple_alpha_blend = 0x400;
            constexpr std::uint32_t k_hair = 0x800;
            constexpr std::uint32_t k_subsurface_scattering_1 = 0x1000;
            constexpr std::uint32_t k_subsurface_scattering_2 = 0x2000;
            constexpr std::uint32_t k_decal_render = 0x4000;
            constexpr std::uint32_t k_decal_normal_render = 0x8000;
            constexpr std::uint32_t k_tessellation_type_mask = 0x30000;
            constexpr std::uint32_t k_tessellation_type_shift = 16;
            constexpr std::uint32_t k_disable_tessellation = 0x40000;
            // [more missing flags]
            constexpr std::uint32_t k_receive_projections = 0x10000000;
            constexpr std::uint32_t k_receive_shadows = 0x20000000;
            constexpr std::uint32_t k_cast_shadows = 0x40000000;
            constexpr std::uint32_t k_dynamic_lighting = 0x80000000;
            constexpr std::uint32_t k_combined_mask = k_irradiance_lighting | k_predator | k_local_reflection_map | k_back_buffer_resolve | k_use_light_map | k_no_deferred_specular | k_no_normal_write | k_depth_occluder | k_stipple_alpha_blend | k_hair | k_subsurface_scattering_1 | k_subsurface_scattering_2 | k_decal_render | k_decal_normal_render | k_tessellation_type_mask | k_disable_tessellation | k_receive_projections | k_receive_shadows | k_cast_shadows | k_dynamic_lighting;
        }
        void read( score::binary_io::input_interface& a_interface, material_data::node_flags& a_flags )
        {
            const auto flags = read< std::uint32_t >( a_interface );
            assert( (flags & ~node_flags::k_combined_mask) == 0 );
            a_flags.irradiance_lighting = check_flag( flags, node_flags::k_irradiance_lighting );
            a_flags.predator = check_flag( flags, node_flags::k_predator );
            a_flags.local_reflection_map = check_flag( flags, node_flags::k_local_reflection_map );
            a_flags.back_buffer_resolve = check_flag( flags, node_flags::k_back_buffer_resolve );
            a_flags.use_light_map = check_flag( flags, node_flags::k_use_light_map );
            a_flags.no_deferred_specular = check_flag( flags, node_flags::k_no_deferred_specular );
            a_flags.no_normal_write = check_flag( flags, node_flags::k_no_normal_write );
            a_flags.depth_occluder = check_flag( flags, node_flags::k_depth_occluder );
            a_flags.stipple_alpha_blend = check_flag( flags, node_flags::k_stipple_alpha_blend );
            a_flags.hair = check_flag( flags, node_flags::k_hair );
            a_flags.subsurface_scattering_1 = check_flag( flags, node_flags::k_subsurface_scattering_1 );
            a_flags.subsurface_scattering_2 = check_flag( flags, node_flags::k_subsurface_scattering_2 );
            a_flags.decal_render = check_flag( flags, node_flags::k_decal_render );
            a_flags.decal_normal_render = check_flag( flags, node_flags::k_decal_normal_render );
            a_flags.tessellation_type = (flags & node_flags::k_tessellation_type_mask) >> node_flags::k_tessellation_type_shift;
            a_flags.disable_tessellation = check_flag( flags, node_flags::k_disable_tessellation );
            a_flags.receive_projections = check_flag( flags, node_flags::k_receive_projections );
            a_flags.receive_shadows = check_flag( flags, node_flags::k_receive_shadows );
            a_flags.cast_shadows = check_flag( flags, node_flags::k_cast_shadows );
            a_flags.dynamic_lighting = check_flag( flags, node_flags::k_dynamic_lighting );
        }

        namespace texture_entry
        {
            constexpr score::bit_range k_type_range{0, 5};
            constexpr score::bit_range k_class_range{5, 3};
        }

        void read_pass( resource::reloc_istream& a_istream, material_data::pass_data& a_pass )
        {
            if( auto* ps = a_istream.try_read_relocation() )
                a_pass.pixel_shader = ps->resource.value();
            if( auto* vs = a_istream.try_read_relocation() )
                a_pass.vertex_shader = vs->resource.value();
            read( a_istream, a_pass.flags );
            const auto read_texture_list = [&a_istream]( material_data::texture_list& a_list )
            {
                const auto texture_count = read< std::uint8_t >( a_istream );
                a_list.textures.reserve( texture_count );
                read( a_istream, a_list.instance_texture_count );
                read( a_istream, a_list.material_texture_count );
                read( a_istream, a_list.first_instance_texture );
                const auto te_scope = resource::reloc_istream_scope{a_istream, "mat.texture_entry"};
                if( !te_scope )
                    return;
                for( std::uint32_t i = 0; i < texture_count; i++ )
                {
                    auto& te = a_list.textures.emplace_back();
                    te.texture = a_istream.read_relocation()->resource.value();
                    read( a_istream, te.mip_map_lod_bias );
                    read( a_istream, te.category );
                    const auto type_and_class = read< std::uint8_t >( a_istream );
                    te.type = static_cast< material_data::texture_type >
                        ( score::get_bits( type_and_class, texture_entry::k_type_range ) );
                    te.class_ = static_cast< texture_class >
                        ( score::get_bits( type_and_class, texture_entry::k_class_range ) );
                    read( a_istream, te.texture_slot );
                    read( a_istream, te.filter );
                }
            };
            const auto read_constant_list = [&a_istream]( material_data::constant_list& a_list )
            {
                const auto constant_count = read< std::uint32_t >( a_istream );
                a_list.constants.reserve( constant_count );
                const auto cl_scope = resource::reloc_istream_scope{ a_istream, "mat.constant_list" };
                if( !cl_scope )
                    return;

                for( std::uint32_t i = 0; i < constant_count; i++ )
                    a_list.constants.push_back( read< float >( a_istream ) );
            };
            read_texture_list( a_pass.pixel_textures );
            read_constant_list( a_pass.pixel_constants );
            read_texture_list( a_pass.vertex_textures );
            read_constant_list( a_pass.vertex_constants );
            read( a_istream, a_pass.pixel_constants.first_instance_param );
            read( a_istream, a_pass.pixel_constants.instance_param_count );
            read( a_istream, a_pass.vertex_constants.first_instance_param );
            read( a_istream, a_pass.vertex_constants.instance_param_count );
            read( a_istream, a_pass.pixel_constants.first_extended_instance_param );
            read( a_istream, a_pass.pixel_constants.extended_instance_param_count );
            read( a_istream, a_pass.vertex_constants.first_extended_instance_param );
            read( a_istream, a_pass.vertex_constants.extended_instance_param_count );
            {
                const auto hdd_scope = resource::reloc_istream_scope{a_istream, "mat.pass.hull_domain_data"};
                if( !hdd_scope )
                    return;
                a_pass.hull_domain_shader_data = material_data::hull_domain_data{};
                if( auto* hs = a_istream.try_read_relocation() )
                    a_pass.hull_domain_shader_data->hull_shader = hs->resource.value();
                if( auto* ds = a_istream.try_read_relocation() )
                    a_pass.hull_domain_shader_data->domain_shader = ds->resource.value();
                read_texture_list( a_pass.hull_domain_shader_data->hull_textures );
                read_constant_list( a_pass.hull_domain_shader_data->hull_constants );
                read_texture_list( a_pass.hull_domain_shader_data->domain_textures );
                read_constant_list( a_pass.hull_domain_shader_data->domain_constants );
                read( a_istream, a_pass.hull_domain_shader_data->fade_start );
                read( a_istream, a_pass.hull_domain_shader_data->fade_range );
            }
        }
    }

    [[nodiscard]] std::unique_ptr< material_data > load_material( resource::reloc_istream& a_istream )
    {
        auto ret = std::make_unique< material_data >();
        read( a_istream, ret->version );
        assert( ret->version == k_version );
        read( a_istream, ret->id );
        read( a_istream, ret->color_write_mask );
        // Skip past blend_mode
        skip_bytes< 0x4 >( a_istream );
        read( a_istream, ret->blend_factor );
        read( a_istream, ret->fog_type_ );
        assert( ret->fog_type_ != material_data::fog_type::invalid );
        read( a_istream, ret->fade_mode_ );
        assert( ret->fade_mode_ != material_data::fade_mode::invalid );

        read( a_istream, ret->flags_ );
        read( a_istream, ret->combined_node_flags );
        read( a_istream, ret->debug_color );
        skip_bytes< 0x4 >( a_istream );
        read( a_istream, ret->alpha_test_ref );
        read( a_istream, ret->depth_write_alpha_threshold );
        read( a_istream, ret->max_lights );
        read( a_istream, ret->max_shadow_lights );
        // Skip past stencil params for now.
        skip_bytes< 0x10 >( a_istream );
        const auto name_offset = read< std::uint32_t >( a_istream );
        // Skip past const char* embedded in material file.
        skip_bytes< 0x4 >( a_istream );
        ret->resource_name = [&a_istream, name_offset]() -> std::string
        {
            if( name_offset == 0u )
                return "<unnamed>";
            const auto resource_name_scope = resource::reloc_istream_scope{a_istream, name_offset, "mat.resource_name"};
            std::string resource_name;
            resource_name.resize( k_resource_name_size );
            a_istream.read( std::as_writable_bytes( std::span{resource_name} ) );
            return resource_name;
        }();
        [[maybe_unused]] const auto magic = read< std::uint32_t >( a_istream );
        assert( magic == k_magic );
        for( std::uint32_t i = 0; i < material_data::k_max_passes; i++ )
        {
            const auto scope = resource::reloc_istream_scope{a_istream, "mat.pass"};
            if( !scope )
                continue;
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
            auto& pass = ret->passes[i];
            pass = material_data::pass_data{};
            read_pass( a_istream, *pass );
        }
        return ret;
    }
}
