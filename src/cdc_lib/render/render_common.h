#ifndef CDC_LIB_RENDER_RENDER_COMMON_H
#define CDC_LIB_RENDER_RENDER_COMMON_H
#include <cstdint>
#include <score/containers/simple_lookup_table.h>

namespace cdc_lib::render
{
    enum class texture_filter
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

    enum class texture_class
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
}

#endif
