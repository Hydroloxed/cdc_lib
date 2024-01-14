#ifndef CDC_LIB_RESOURCE_TRAS_PC_W_TRAS_PC_W_RELOCATION_H
#define CDC_LIB_RESOURCE_TRAS_PC_W_TRAS_PC_W_RELOCATION_H
#include <cdc_lib/resource/rsrc_relocation.h>
#include <score/binary_io/binary_io.h>
#include <stdexcept>
#include <vector>

namespace cdc_lib::resource::tras::pc_w
{
    class relocation_table_invalid : public std::runtime_error
    {
    public:
        explicit relocation_table_invalid( const std::string& a_message ) :
            std::runtime_error{a_message}
        {}
    };
    [[nodiscard]] std::vector< cooked_relocation > load_relocation_table( score::binary_io::input_interface& input_interface );
}

#endif

