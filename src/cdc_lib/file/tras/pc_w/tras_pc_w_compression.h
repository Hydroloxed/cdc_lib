#ifndef CDC_LIB_FILE_TRAS_PC_W_TRAS_PC_W_COMPRESSION_H
#define CDC_LIB_FILE_TRAS_PC_W_TRAS_PC_W_COMPRESSION_H
#include <score/binary_io/binary_io.h>

namespace cdc_lib::file::tras::pc_w
{
    [[nodiscard]] bool is_cdrm( score::binary_io::input_interface& a_input );
    [[nodiscard]] std::string decompress_cdrm( score::binary_io::input_interface& a_input );
}

#endif
