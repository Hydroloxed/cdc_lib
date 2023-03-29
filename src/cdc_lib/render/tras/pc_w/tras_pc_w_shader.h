#ifndef CDC_LIB_RENDER_TRAS_PC_W_TRAS_PC_W_SHADER_H
#define CDC_LIB_RENDER_TRAS_PC_W_TRAS_PC_W_SHADER_H
#include <cstdint>
#include <memory>
#include <score/binary_io/binary_io.h>
#include <vector>

namespace cdc_lib::render::tras::pc_w
{
    enum class shader_type : std::uint32_t
    {
        unknown,
        pixel_shader,
        vertex_shader,
        hull_shader,
        domain_shader,
        compute_shader,
        geometry_shader,
        max
    };

    struct shader_id
    {
        std::uint32_t hi;
        std::uint32_t size;
        std::uint64_t lo;
    };

    struct shader
    {
        shader_id id;
        std::vector< std::byte > data;
    };

    struct shader_table
    {
        shader_type type;
        std::vector< std::shared_ptr< shader > > shaders;
    };

    std::unique_ptr< shader_table > load_shader_table( score::binary_io::input_interface& input );
}

#endif
