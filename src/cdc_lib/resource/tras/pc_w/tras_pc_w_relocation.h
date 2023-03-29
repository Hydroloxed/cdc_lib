#ifndef CDC_LIB_RESOURCE_TRAS_PC_W_TRAS_PC_W_RELOCATION_H
#define CDC_LIB_RESOURCE_TRAS_PC_W_TRAS_PC_W_RELOCATION_H
#include <cdc_lib/resource/rsrc_relocation.h>
#include <score/binary_io/binary_io.h>
#include <vector>

namespace cdc_lib::resource::tras::pc_w
{
    [[nodiscard]] std::vector< cooked_relocation > load_relocation_table( score::binary_io::input_interface& input_interface );
}

#endif

