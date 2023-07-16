#ifndef CDC_LIB_RESOURCE_TRAS_PC_W_TRAS_PC_W_RESOLVE_OBJECT_H
#define CDC_LIB_RESOURCE_TRAS_PC_W_TRAS_PC_W_RESOLVE_OBJECT_H
#include <cdc_lib/resource/rsrc_resolve_object.h>
#include <memory>
#include <score/binary_io/binary_io.h>

namespace cdc_lib::resource::tras::pc_w
{
    struct cooked_resolve_section_extra_data
    {
        std::uint32_t unique_id{0};
        std::uint32_t packed_offset{0};
        std::uint32_t compressed_size{0};
        std::uint32_t decompressed_offset{0};
    };

    std::unique_ptr< cooked_resolve_object > load_object( score::binary_io::input_interface& input_interface );
}

#endif
