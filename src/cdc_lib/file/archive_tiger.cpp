#include "archive_fs.h"
#include "cdc_lib/core/core_crc32.h"
#include <cdc_lib/file/archive_fs.h>
#include <fmt/format.h>
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
        ret.config_name = ret.config_name.substr( 0, ret.config_name.find( '\0' ) );
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

    [[nodiscard]] bool can_read_offset( const archive& a_archive, std::uint32_t a_offset )
    {
        const auto dlc_index = score::get_bits( a_offset, k_offset_bit_range_dlc_index );
        return dlc_index == a_archive.dlc_index;
    }

    [[nodiscard]] std::string read_offset( const archive& a_archive, std::uint32_t a_offset, std::size_t a_size )
    {
        const auto actual_archive_file = get_effective_archive_path( a_archive, a_offset );
        const auto dlc_index = score::get_bits( a_offset, k_offset_bit_range_dlc_index );
        if( dlc_index != a_archive.dlc_index )
            throw std::runtime_error{fmt::format( "Wrong archive ({:#x}), for offset ({:#x})", dlc_index, a_offset )};

        std::string data( a_size, '\0' );
        auto stream = std::ifstream{actual_archive_file, std::ios::in | std::ios::binary};
        if( !stream.good() )
            throw std::runtime_error{fmt::format( "Could not open file {}", actual_archive_file.string() )};
        stream.exceptions( std::ifstream::failbit | std::ifstream::badbit );
        stream.seekg( a_offset & k_offset_offset_mask );
        stream.read( data.data(), static_cast< std::streamsize >( a_size ) );
        return data;
    }

    [[nodiscard]] std::string read_record( const archive& a_archive, const archive_record& a_record )
    {
        return read_offset( a_archive, a_record.offset, a_record.size );
    }

    [[nodiscard]] std::uint64_t hash_filename( const archive& a_archive, std::string_view a_filename )
    {
        const std::string real_filename = a_archive.config_name + "\\" + std::string{a_filename};
        return core::crc32(real_filename);
    }

    [[nodiscard]] bool can_read_offset( const archive_multifs& a_multifs, std::uint32_t a_offset )
    {
        const auto dlc_index = score::get_bits( a_offset, k_offset_bit_range_dlc_index );
        return a_multifs.archives.contains( dlc_index );
    }

    [[nodiscard]] std::string read_offset( const archive_multifs& a_multifs, std::uint32_t a_offset, std::size_t a_size )
    {
        if( !can_read_offset( a_multifs, a_offset ) )
            throw std::runtime_error{fmt::format( "Invalid offset {:#x}, no archive can read dlcindex {:#x}",
                                                  a_offset,
                                                  score::get_bits( a_offset, k_offset_bit_range_dlc_index ) )};
        assert( can_read_offset( a_multifs, a_offset ) );
        const auto dlc_index = score::get_bits( a_offset, k_offset_bit_range_dlc_index );
        return read_offset( a_multifs.archives.at( dlc_index ), a_offset, a_size );
    }

    [[nodiscard]] std::string read_record( const archive_multifs& a_multifs, const archive_record& a_record )
    {
        return read_offset( a_multifs, a_record.offset, a_record.size );
    }

    [[nodiscard]] archive_multifs make_multifs_tras( const std::filesystem::path& a_game_path )
    {
        archive_multifs ret{};
        ret.gamepath = a_game_path.string();
        const std::array known_archives =
        {
            "bigfile.000.tiger",
            // "bigfile_ENGLISH.000.tiger" - TODO: add support for localized bigfiles
            "patch.000.tiger",
            "patch2.000.tiger",
            "patch3.000.tiger",
            "title.000.tiger",
            "DLC/PACK1.000.tiger",
            "DLC/PACK2.000.tiger",
            "DLC/PACK3.000.tiger",
            "DLC/PACK4.000.tiger",
            "DLC/PACK5.000.tiger",
            "DLC/PACK6.000.tiger",
            "DLC/PACK7.000.tiger",
            "DLC/PACK8.000.tiger"
        };
        for( const auto& path : known_archives )
        {
            if( std::filesystem::exists( a_game_path / path ) )
            {
                const auto interface = score::binary_io::open_file( (a_game_path / path).string() );
                const auto archive = cdc_lib::file::load_tiger_archive( *interface, a_game_path / path );
                ret.archives[archive.dlc_index] = archive;
            }
        }
        return ret;
    }
}
