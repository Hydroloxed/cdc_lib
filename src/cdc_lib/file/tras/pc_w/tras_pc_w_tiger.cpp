#include "tras_pc_w_tiger.h"
#include "score/binary_io/binio_strings.h"
#include <cassert>
#include <utility>

namespace cdc_lib::file::tras::pc_w
{
    namespace
    {
        constexpr std::uint32_t k_magic = 0x53464154u;
        constexpr std::uint32_t k_version = 0x3u;
        constexpr std::uint32_t k_config_length = 32u;
    }

    [[nodiscard]] archive load_archive( score::binary_io::input_interface& a_input, std::filesystem::path a_path )
    {
        archive ret{};
        ret.archive_path = std::move( a_path );
        const auto magic = read< std::uint32_t >( a_input );
        assert( magic == k_magic );
        const auto version = read< std::uint32_t >( a_input );
        assert( version == k_version );
        ret.archive_count = read< std::uint32_t >( a_input );
        const auto record_count = read< std::uint32_t >( a_input );
        ret.dlc_index = read< std::uint32_t >( a_input );
        read_fixed_string( a_input, ret.config_name, k_config_length );
        for( std::uint32_t i = 0; i < record_count; ++i )
        {
            archive_record record{};
            record.name_hash = read< std::uint32_t >( a_input );
            record.spec_mask = read< std::uint32_t >( a_input );
            record.size = read< std::uint32_t >( a_input );
            record.offset = read< std::uint32_t >( a_input );
            ret.records.push_back( record );
        }
        return ret;
    }
}
