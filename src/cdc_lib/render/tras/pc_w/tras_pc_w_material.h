#ifndef CDC_LIB_RENDER_PC_W_TRAS_PC_W_TRAS_MATERIAL_H
#define CDC_LIB_RENDER_PC_W_TRAS_PC_W_TRAS_MATERIAL_H
#include <array>
#include <cdc_lib/render/render_common.h>
#include <cdc_lib/resource/rsrc_reloc_input_stream.h>
#include <cdc_lib/resource/rsrc_resource.h>
#include <cstdint>
#include <memory>
#include <optional>
#include <score/containers/simple_lookup_table.h>
#include <string>

namespace cdc_lib::render::tras::pc_w
{
    struct material_data
    {
        enum class fog_type : std::uint16_t
        {
            none,
            linear_dist,
            non_linear_dist,
            non_linear_dist_and_height,
            invalid
        };
        constexpr static score::simple_lookup_table< fog_type, const char*, 4 > fog_type_debugstr =
        {
            std::pair{ fog_type::none, "none" },
            std::pair{ fog_type::linear_dist, "linear_dist" },
            std::pair{ fog_type::non_linear_dist, "non_linear_dist" },
            std::pair{ fog_type::non_linear_dist_and_height, "non_linear_dist_and_height" }
        };

        enum class fade_mode : std::uint16_t
        {
            off,
            alpha_blend,
            alpha_test,
            alpha_to_mask,
            alpha_to_mask_dither,
            clip_or_blend,
            invalid
        };
        constexpr static score::simple_lookup_table< fade_mode, const char*, 6 > fade_mode_debugstr =
        {
            std::pair{ fade_mode::off, "off" },
            std::pair{ fade_mode::alpha_blend, "alpha_blend" },
            std::pair{ fade_mode::alpha_test, "alpha_test" },
            std::pair{ fade_mode::alpha_to_mask, "alpha_to_mask" },
            std::pair{ fade_mode::alpha_to_mask_dither, "alpha_to_mask_dither" },
            std::pair{ fade_mode::clip_or_blend, "clip_or_blend" }
        };

        enum class texture_type : std::uint8_t
        {
            material_texture,
            instance_texture_1,
            instance_texture_2,
            instance_texture_3,
            instance_texture_4,
            light_map_texture,
            global_texture_1,
            global_texture_2,
            global_texture_3,
            global_texture_4,
            back_buffer,
            depth_buffer,
            normal_buffer,
            deferred_buffer,
            max
        };
        constexpr static score::simple_lookup_table< texture_type, const char*, 14 > texture_type_debugstr =
        {
            std::pair{texture_type::material_texture, "material_texture"},
            std::pair{texture_type::instance_texture_1, "instance_texture_1"},
            std::pair{texture_type::instance_texture_2, "instance_texture_2"},
            std::pair{texture_type::instance_texture_3, "instance_texture_3"},
            std::pair{texture_type::instance_texture_4, "instance_texture_4"},
            std::pair{texture_type::light_map_texture, "light_map_texture"},
            std::pair{texture_type::global_texture_1, "global_texture_1"},
            std::pair{texture_type::global_texture_2, "global_texture_2"},
            std::pair{texture_type::global_texture_3, "global_texture_3"},
            std::pair{texture_type::global_texture_4, "global_texture_4"},
            std::pair{texture_type::back_buffer, "back_buffer"},
            std::pair{texture_type::depth_buffer, "depth_buffer"},
            std::pair{texture_type::normal_buffer, "normal_buffer"},
            std::pair{texture_type::deferred_buffer, "deferred_buffer"}
        };

        struct node_flags
        {
            bool irradiance_lighting : 1;
            bool predator : 1;
            bool local_reflection_map : 1;
            bool back_buffer_resolve : 1;
            bool use_light_map : 1;
            bool no_deferred_specular : 1;
            bool no_normal_write : 1;
            bool depth_occluder : 1;
            bool stipple_alpha_blend : 1;
            bool hair : 1;
            bool subsurface_scattering_1 : 1;
            bool subsurface_scattering_2 : 1;
            bool decal_render : 1;
            bool decal_normal_render : 1;
            unsigned tessellation_type : 2;
            bool disable_tessellation : 1;
            bool receive_projections : 1;
            bool receive_shadows : 1;
            bool cast_shadows : 1;
            bool dynamic_lighting : 1;
        };

        struct texture_entry
        {
            resource::resource_ref texture{};
            float mip_map_lod_bias{};
            std::uint32_t category{0};
            texture_type type{};
            texture_class class_{};
            std::uint8_t texture_slot{};
            texture_filter filter{};
        };

        struct texture_list
        {
            std::uint8_t instance_texture_count{0};
            std::uint8_t material_texture_count{0};
            std::uint8_t first_instance_texture{0};
            std::vector< texture_entry > textures{};
        };

        struct constant_list
        {
            std::vector< float > constants{};
            std::uint8_t first_instance_param{0};
            std::uint8_t instance_param_count{0};
            std::uint8_t first_extended_instance_param{0};
            std::uint8_t extended_instance_param_count{0};
        };

        struct hull_domain_data
        {
            resource::resource_ref hull_shader{};
            resource::resource_ref domain_shader{};
            texture_list  hull_textures{};
            constant_list hull_constants{};
            texture_list  domain_textures{};
            constant_list domain_constants{};
            float fade_start{0.0f};
            float fade_range{0.0f};
        };

        constexpr static std::uint32_t k_pass_invalid = static_cast< std::uint32_t >( -1 );
        constexpr static std::uint32_t k_pass_position_only = 0;
        constexpr static std::uint32_t k_pass_alpha = 1;
        constexpr static std::uint32_t k_pass_shadow_to_color = 2;
        constexpr static std::uint32_t k_pass_opaque = 3;
        constexpr static std::uint32_t k_pass_translucent_bloom = 4;
        constexpr static std::uint32_t k_pass_depth_to_color = 5;
        // Don't know what happened to pass 6, no name for it is present in debug info,
        // neither in final nor debug builds.
        constexpr static std::uint32_t k_pass_normal = 7;
        constexpr static std::uint32_t k_pass_translucent = 8;
        constexpr static std::uint32_t k_pass_debug = 9;
        constexpr static score::simple_lookup_table< std::uint32_t, const char*, 9 > pass_index_debugstr =
        {
            std::pair{k_pass_position_only, "position_only"},
            std::pair{k_pass_alpha, "alpha"},
            std::pair{k_pass_shadow_to_color, "shadow_to_color"},
            std::pair{k_pass_opaque, "opaque"},
            std::pair{k_pass_translucent_bloom, "translucent_bloom"},
            std::pair{k_pass_depth_to_color, "depth_to_color"},
            std::pair{k_pass_normal, "normal"},
            std::pair{k_pass_translucent, "translucent"},
            std::pair{k_pass_debug, "debug"}
        };
        struct pass_data
        {
            resource::resource_ref pixel_shader;
            resource::resource_ref vertex_shader;
            node_flags flags;
            texture_list  pixel_textures;
            constant_list pixel_constants;
            texture_list  vertex_textures;
            constant_list vertex_constants;
            std::optional< hull_domain_data > hull_domain_shader_data;
        };

        constexpr static std::uint32_t k_version{0x13};
        std::uint32_t version{k_version};
        std::uint32_t id{0};
        std::uint32_t color_write_mask{0};
        blend_mode blend_mode_;
        std::uint32_t blend_factor{0};
        fog_type fog_type_{fog_type::none};
        fade_mode fade_mode_{fade_mode::off};
        struct
        {
            bool highlight : 1;
            bool alpha_highlight : 1;
            bool fog : 1;
            bool affects_position : 1;
            bool double_sided : 1;
            bool _2d : 1;
            bool translucent_depth_write : 1;
            bool back_before_front : 1;
            bool sunlight_enabled : 1;
            bool draw_back_face_only : 1;
            bool pre_translucent_render : 1;
            bool tessellation : 1;
        } flags;
        node_flags combined_node_flags;
        std::uint32_t debug_color;
        blend_mode alpha_bloom_blend_mode;
        std::uint8_t alpha_test_ref;
        std::uint8_t depth_write_alpha_threshold;
        std::uint8_t max_lights;
        std::uint8_t max_shadow_lights;
        // TODO: stencil params
        std::string resource_name{"<unnamed>"};
        constexpr static std::uint32_t k_max_passes{10};
        std::array< std::optional< pass_data >, k_max_passes > passes;
    };

    [[nodiscard]] std::unique_ptr< material_data > load_material( resource::reloc_istream& istream );
}

#endif
