#ifndef CDC_LIB_FILE_TRAS_PC_W_TRAS_PC_W_TIGER_H
#define CDC_LIB_FILE_TRAS_PC_W_TRAS_PC_W_TIGER_H
#include <cdc_lib/file/archive_fs.h>
#include <score/binary_io/binary_io.h>

namespace cdc_lib::file::tras::pc_w
{
    [[nodiscard]] archive load_archive( score::binary_io::input_interface& a_input, std::filesystem::path a_path );
}

#endif
