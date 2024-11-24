#include "archive_fs.h"
#include <cdc_lib/file/archive_fs.h>
#include <fmt/core.h>
#include <fstream>
#include <score/binary_io/binio_strings.h>
#include <score/score_bit.h>
#include <string>

namespace cdc_lib::file
{
    namespace
    {
        constexpr std::uint32_t k_magic = 0x53464154u;
        constexpr std::uint32_t k_version = 0x3u;
        constexpr std::uint32_t k_config_length = 32u;
        constexpr std::uint32_t k_offset_offset_mask = 0xfffff800;
        constexpr auto k_offset_bit_range_dlc_index = score::bit_range{4, 7};
        constexpr auto k_offset_bit_range_archive_index = score::bit_range{0, 4};

        [[nodiscard]] std::filesystem::path get_effective_archive_path( const archive& a_archive, std::uint32_t a_offset )
        {
            const auto archive_index = score::get_bits( a_offset, k_offset_bit_range_archive_index );

            // Replace '*.000.tiger' with '*.{:03}.tiger'
            auto new_path = a_archive.archive_path.generic_string();
            constexpr auto k_extension_length = sizeof ".000.tiger";
            const auto extension = new_path.find( ".000.tiger" );
            const auto new_extension = fmt::format( ".{:03}.tiger", archive_index );
            new_path.replace( extension, k_extension_length, new_extension );
            return new_path;
        }
    }

    [[nodiscard]] archive load_tiger_archive( score::binary_io::input_interface& a_input, const std::filesystem::path& a_path )
    {
        archive ret{};
        if( !a_path.filename().string().ends_with( ".tiger" ) )
            throw std::runtime_error{fmt::format( "File '{}' does not end with .tiger", a_path.string() )};
        if( !a_path.filename().string().ends_with( ".000.tiger" ) )
            throw std::runtime_error{fmt::format( "File '{}' is not the 0th archive", a_path.string() )};
        ret.archive_path = a_path;
        const auto magic = read< std::uint32_t >( a_input );
        if( magic != k_magic )
            throw std::runtime_error{fmt::format( "Invalid magic, expected {:#x}, got {:#x}", k_magic, magic )};
        const auto version = read< std::uint32_t >( a_input );
        if( version != k_version )
            throw std::runtime_error{fmt::format( "Invalid version, got {}. For now only TRAS (v3) archives are supported", version )};
        ret.archive_count = read< std::uint32_t >( a_input );
        const auto record_count = read< std::uint32_t >( a_input );
        ret.dlc_index = read< std::uint32_t >( a_input );
        ret.config_name = read_fixed_string( a_input, k_config_length );
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

    [[nodiscard]] std::string read_offset( const archive& a_archive, std::uint32_t a_offset, std::size_t a_size )
    {
        const auto actual_archive_file = get_effective_archive_path( a_archive, a_offset );
        const auto dlc_index = score::get_bits( a_offset, k_offset_bit_range_dlc_index );
        if( dlc_index != a_archive.dlc_index )
            throw std::runtime_error{fmt::format( "Wrong archive ({:#x}), for offset ({:#x})", dlc_index, a_offset )};

        std::string data( a_size, '\0' );
        auto stream = std::ifstream{actual_archive_file};
        if( !stream.good() )
            throw std::runtime_error{fmt::format( "Could not open file {}", actual_archive_file.c_str() )};
        stream.exceptions( std::ifstream::failbit | std::ifstream::badbit );
        stream.seekg( a_offset & k_offset_offset_mask );
        stream.read( data.data(), static_cast< std::streamsize >( a_size ) );
        return data;
    }

    [[nodiscard]] std::string read_record( const archive& a_archive, const archive_record& a_record )
    {
        return read_offset( a_archive, a_record.offset, a_record.size );
    }
}
