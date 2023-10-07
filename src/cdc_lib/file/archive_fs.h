#ifndef CDC_LIB_FILE_ARCHIVE_FS_H
#define CDC_LIB_FILE_ARCHIVE_FS_H
#include <cstdint>
#include <filesystem>
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
}

#endif
