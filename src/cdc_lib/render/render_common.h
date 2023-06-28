#ifndef CDC_LIB_RENDER_RENDER_COMMON_H
#define CDC_LIB_RENDER_RENDER_COMMON_H
#include <cstdint>
#include <score/containers/simple_lookup_table.h>

namespace cdc_lib::render
{
    enum class texture_filter : std::uint8_t
    {
        point,
        bilinear,
        trilinear,
        anisotropic_1x,
        anisotropic_2x,
        anisotropic_4x,
        anisotropic_8x,
        anisotropic_16x,
        best,
        default_,
        invalid
    };

    enum class texture_class : std::uint8_t
    {
        unknown,
        _2d,
        _3d,
        cube,
        normal_map,
        vertex
    };
    static constexpr score::simple_lookup_table< texture_class, const char*, 5 > texture_class_debugstr =
    {
        std::pair{texture_class::_2d, "2d"},
        std::pair{texture_class::_3d, "3d"},
        std::pair{texture_class::cube, "cube"},
        std::pair{texture_class::normal_map, "normal_map"},
        std::pair{texture_class::vertex, "vertex"}
    };

    struct blend_mode
    {
        enum class blend_op : std::uint8_t
        {
            add,
            subtract,
            rev_subtract,
            min,
            max,
            clip
        };

        enum class blend_arg : std::uint8_t
        {
            zero,
            one,
            src_color,
            inv_src_color,
            src_alpha,
            inv_src_alpha,
            dst_color,
            inv_dst_color,
            dst_alpha,
            inv_dst_alpha,
            factor,
            inv_factor
        };

        enum class compare_func : std::uint8_t
        {
            never,
            less,
            equal,
            less_equal,
            greater,
            not_equal,
            greater_equal,
            always
        };

        // TODO: properly define members for this type
    };
}

#endif
