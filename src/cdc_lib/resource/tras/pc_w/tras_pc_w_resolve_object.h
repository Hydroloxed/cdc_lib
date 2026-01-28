#ifndef CDC_LIB_RESOURCE_TRAS_PC_W_TRAS_PC_W_RESOLVE_OBJECT_H
#define CDC_LIB_RESOURCE_TRAS_PC_W_TRAS_PC_W_RESOLVE_OBJECT_H
#include <cdc_lib/resource/rsrc_resolve_object.h>
#include <memory>
#include <score/binary_io/binary_io.h>

namespace cdc_lib::resource::tras::pc_w
{

    std::unique_ptr< cooked_resolve_object > load_object( score::binary_io::input_interface& a_input_interface );
    void write_object( score::binary_io::output_interface& a_output,
                       const cooked_resolve_object& a_object,
                       const std::vector< std::string >& a_section_datas );
}

#endif
