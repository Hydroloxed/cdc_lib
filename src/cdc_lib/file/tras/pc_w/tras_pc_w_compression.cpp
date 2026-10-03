#include "tras_pc_w_compression.h"
#include <cassert>
#include <fmt/core.h>
#include <score/binary_io/binio_strings.h>
#include <score/score_bit.h>
#include <stdexcept>
#include <vector>
#include <zlib.h>

namespace cdc_lib::file::tras::pc_w
{
    namespace
    {
        enum class block_type
        {
            empty,
            uncompressed,
            zip_compressed,
            x_compressed
        };

        struct block
        {
            std::size_t uncompressed_size{0};
            std::size_t compressed_size{0};
            block_type type{};
        };

        constexpr std::string_view k_magic = "CDRM";
        constexpr std::string_view k_next_magic = "NEXT";
        constexpr std::uint32_t k_version = 0x0;
        constexpr std::uint32_t k_max_compressed_size = 0x40000;
        constexpr std::uint32_t k_max_uncompressed_size = 0x40000;
        constexpr std::uint32_t k_block_alignment = 0x10;

        constexpr score::bit_range k_block_type_bit_range = score::bit_range{0, 8};
        constexpr score::bit_range k_uncompressed_size_bit_range = score::bit_range{8, 24};

        std::string decompress( const std::string& a_compressed, const block& a_block )
        {
            switch( a_block.type )
            {
                case block_type::empty:
                    throw std::runtime_error{"Empty block type, can this really happen?"};
                case block_type::uncompressed:
                    return a_compressed;
                case block_type::zip_compressed:
                {
                    z_stream stream{};
                    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
                    stream.zalloc = Z_NULL;
                    stream.zfree = Z_NULL;
                    stream.opaque = Z_NULL;
                    [[maybe_unused]] int result = inflateInit( &stream );
                    assert( result == Z_OK );
                    // NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast)
                    stream.next_in = reinterpret_cast< const unsigned char* >( a_compressed.data() );
                    stream.avail_in = a_block.compressed_size;
                    std::string uncompressed( a_block.uncompressed_size, '\0' );
                    stream.next_out = reinterpret_cast< unsigned char* >( uncompressed.data() );
                    // NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)
                    stream.avail_out = a_block.uncompressed_size;

                    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
                    result = inflate( &stream, Z_NO_FLUSH );
                    assert( result == Z_STREAM_END );
                    // assert( stream.avail_in == 0 );
                    assert( stream.avail_out == 0 );
                    assert( stream.total_out == a_block.uncompressed_size );
                    assert( inflateEnd( &stream ) == Z_OK );
                    return uncompressed;
                }
                case block_type::x_compressed:
                    throw std::runtime_error{"X-compressed blocks are not supported yet"};
                default:
                    throw std::runtime_error{"Unknown block type"};
            }
        }

        // TODO: put this function in score.binary_io
        void align_to( score::binary_io::input_interface& a_input, std::size_t a_alignment )
        {
            assert( std::popcount( a_alignment ) == 1 );
            const auto current_offset = a_input.tell();
            if( current_offset % a_alignment == 0 )
                return;
            const auto padding_bytes = a_alignment - ( current_offset % a_alignment );
            a_input.seek( current_offset + padding_bytes );
            assert( a_input.tell() % a_alignment == 0 );
        }
    }
    [[nodiscard]] bool is_cdrm( score::binary_io::input_interface& a_input )
    {
        std::string magic = read_fixed_string( a_input, 4 );
        a_input.seek( a_input.tell() - 4 );
        return magic == k_magic;
    }

    std::string decompress_cdrm( score::binary_io::input_interface& a_input )
    {
        if( !is_cdrm( a_input ) )
            throw std::runtime_error{"Not a CDRM stream"};

        assert( a_input.tell() % k_block_alignment == 0 && "we assume that CDRM files are aligned" );
        skip_bytes< 4 >( a_input );
        const auto version = read< std::uint32_t >( a_input );
        if( version != k_version )
            throw std::runtime_error{fmt::format( "Wrong CDRM version: was {:08x}, wanted {:08x}", version, k_version )};
        const auto block_count = read< std::uint32_t >( a_input );
        auto padding_bytes_count = read< std::uint32_t >( a_input );
        assert( padding_bytes_count == 0 ); // probably not used...
        // ... but try to do it anyway
        while( padding_bytes_count-- )
            skip_bytes< 1 >( a_input );
        std::string out_data{};
        std::vector< block > blocks;
        for( std::uint32_t i = 0; i < block_count; ++i )
        {
            auto b = block{};
            const auto packed = read< std::uint32_t >( a_input );
            b.type = static_cast< block_type >( score::get_bits( packed, k_block_type_bit_range ) );
            b.uncompressed_size = score::get_bits( packed, k_uncompressed_size_bit_range );
            b.compressed_size = read< std::uint32_t >( a_input );
            if( b.uncompressed_size > k_max_uncompressed_size )
                throw std::runtime_error{fmt::format( "Uncompressed size {:#x} too big (max {:#x}) (corrupt CDRM)",
                                                      b.uncompressed_size,
                                                      k_max_uncompressed_size )};
            if( b.compressed_size > k_max_compressed_size )
                throw std::runtime_error{fmt::format( "Compressed size {:#x} too big (max {:#x}) (corrupt CDRM)",
                                                      b.compressed_size,
                                                      k_max_compressed_size )};
            blocks.push_back( b );
        }
        align_to( a_input, k_block_alignment );
        for( const auto& b : blocks )
        {
            std::string compressed = read_fixed_string( a_input, b.compressed_size );
            out_data += decompress( compressed, b );
            if( &b != &blocks.back() )
                align_to( a_input, k_block_alignment );
        }
        try
        {
            [[maybe_unused]] std::string next_magic = read_fixed_string( a_input, 4 );
            assert( next_magic == k_next_magic );
        }
        catch( ... )
        {
            // we can safely ignore,
            // the data probably just didn't include the NEXT marker
        }

        return out_data;
    }
}
