#ifndef CDC_LIB_FILE_ARCHIVE_FS_H
#define CDC_LIB_FILE_ARCHIVE_FS_H
#include <cstdint>
#include <filesystem>
#include <map>
#include <score/binary_io/binary_io.h>
#include <string>
#include <vector>

namespace cdc_lib::file
{
    struct archive_record
    {
        std::uint64_t name_hash{};
        std::uint64_t spec_mask{};
        std::size_t size{};
        std::uint64_t offset{};
    };

    struct archive
    {
        std::vector< archive_record > records{};
        std::uint32_t archive_count{};
        std::uint32_t dlc_index{};
        std::string config_name{};
        std::filesystem::path archive_path{};
    };

    [[nodiscard]] archive load_tiger_archive( score::binary_io::input_interface& a_input, const std::filesystem::path& a_path );
    [[nodiscard]] bool can_read_offset( const archive& a_archive, std::uint32_t a_offset );
    [[nodiscard]] std::string read_offset( const archive& a_archive, std::uint32_t a_offset, std::size_t a_size );
    [[nodiscard]] std::string read_record( const archive& a_archive, const archive_record& a_record );

    struct archive_multifs
    {
        std::string gamepath{};
        std::map< std::uint32_t, archive > archives{};
    };

    [[nodiscard]] bool can_read_offset( const archive_multifs& a_multifs, std::uint32_t a_offset );
    [[nodiscard]] std::string read_offset( const archive_multifs& a_multifs, std::uint32_t a_offset, std::size_t a_size );
    [[nodiscard]] std::string read_record( const archive_multifs& a_multifs, const archive_record& a_record );
    [[nodiscard]] archive_multifs make_multifs_tras( const std::filesystem::path& a_game_path );
}

#endif
