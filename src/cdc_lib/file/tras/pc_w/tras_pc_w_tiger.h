#ifndef CDC_LIB_FILE_TRAS_PC_W_TRAS_PC_W_TIGER_H
#define CDC_LIB_FILE_TRAS_PC_W_TRAS_PC_W_TIGER_H
#include <cdc_lib/file/archive_fs.h>
#include <score/binary_io/binary_io.h>

namespace cdc_lib::file::tras::pc_w
{
    [[nodiscard]] archive load_archive( score::binary_io::input_interface& a_input, const std::filesystem::path& a_path );
    [[nodiscard]] std::string read_offset( const archive& a_archive, std::uint32_t a_offset, std::size_t a_size );
    [[nodiscard]] std::string read_record( const archive& a_archive, const archive_record& a_record );
}

#endif
