#include "tras_pc_w_compression.h"
#include <cassert>
#include <fmt/core.h>
#include <score/binary_io/binary_io.h>
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
            x_compressed,
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

        constexpr score::bit_range k_block_type_bit_range{0, 8};
        constexpr score::bit_range k_uncompressed_size_bit_range{8, 24};

        void throw_on_error( int a_result, int a_expected )
        {
            if( a_result == a_expected )
                return;

            switch( a_result )
            {
            case Z_OK:
                // In this case we probably wanted Z_STREAM_END
                throw std::runtime_error{"Stream ended too early. "
                                         "Mismatch between CDRM and zlib metadata?"};
            case Z_NEED_DICT:
                throw std::invalid_argument{"Invalid CDRM: Dictionary missing"};
            case Z_DATA_ERROR:
                throw std::invalid_argument{"Corrupt CDRM"};
            case Z_STREAM_ERROR:
                assert(false && "Bug in decompress(), got Z_STREAM_ERROR. "
                                "Arguments passed to inflate() or inflateInit() were bad");
                break;
            case Z_MEM_ERROR:
                throw std::bad_alloc{};
            case Z_BUF_ERROR:
                throw std::invalid_argument{"Corrupt CDRM"};
            default:
                assert(false && "Bug in decompress(), got unknown ZLIB error");
                throw std::invalid_argument{"Corrupt CDRM, got unknown zlib error"};
            }
        }

        std::string decompress( const std::string& a_compressed, const block& a_block )
        {
            switch( a_block.type )
            {
                case block_type::empty:
                    throw std::runtime_error{"Empty block type, can this really happen?"};
                case block_type::uncompressed:
                    if( a_block.compressed_size != a_block.uncompressed_size )
                        throw std::invalid_argument{"CDRM corrupt, uncompressed block sizes do not match"};
                    return a_compressed;
                case block_type::zip_compressed:
                {
                    z_stream stream{};
                    stream.zalloc = Z_NULL;
                    stream.zfree = Z_NULL;
                    stream.opaque = Z_NULL;
                    [[maybe_unused]] int result = inflateInit( &stream );
                    throw_on_error(result, /*a_expected=*/Z_OK);

                    // NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast)
                    stream.next_in = reinterpret_cast< const Bytef* >( a_compressed.data() );
                    std::string uncompressed( a_block.uncompressed_size, '\0' );
                    stream.next_out = reinterpret_cast< Bytef* >( uncompressed.data() );
                    // NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)
                    stream.avail_in = a_block.compressed_size;
                    stream.avail_out = a_block.uncompressed_size;

                    result = inflate( &stream, Z_NO_FLUSH );
                    throw_on_error(result, /*a_expected=*/Z_STREAM_END);

                    if( stream.total_out != a_block.uncompressed_size )
                        throw std::invalid_argument{"CDRM corrupt, inflate stream was too short"};
                    if( stream.avail_in != 0 )
                        throw std::invalid_argument{"Garbage at end of zlib block"};
                    assert( stream.avail_out == 0 );

                    result = inflateEnd( &stream );
                    throw_on_error(result, /*a_expected=*/Z_OK);
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

        block read_block( score::binary_io::input_interface& a_input )
        {
            block b;
            const auto packed = read< std::uint32_t >( a_input );
            b.type = static_cast< block_type >( score::get_bits( packed, k_block_type_bit_range ) );
            b.uncompressed_size = score::get_bits( packed, k_uncompressed_size_bit_range );
            b.compressed_size = read< std::uint32_t >( a_input );
            return b;
        }
    }
    [[nodiscard]] bool is_cdrm( score::binary_io::input_interface& a_input )
    {
        const std::string magic = read_fixed_string( a_input, 4 );
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
            auto b = read_block( a_input );
            if( b.uncompressed_size > k_max_uncompressed_size )
                throw std::length_error{fmt::format( "Uncompressed size {:#x} too big (max {:#x}) (corrupt CDRM)",
                                                      b.uncompressed_size,
                                                      k_max_uncompressed_size )};
            if( b.compressed_size > k_max_compressed_size )
                throw std::length_error{fmt::format( "Compressed size {:#x} too big (max {:#x}) (corrupt CDRM)",
                                                      b.compressed_size,
                                                      k_max_compressed_size )};
            blocks.push_back( b );
        }
        align_to( a_input, k_block_alignment );
        for( const auto& b : blocks )
        {
            const std::string compressed = read_fixed_string( a_input, b.compressed_size );
            out_data += decompress( compressed, b );
            if( &b != &blocks.back() )
                align_to( a_input, k_block_alignment );
        }
        [[maybe_unused]] std::string next_magic;
        try
        {
            next_magic = read_fixed_string( a_input, 4 );
        }
        catch( ... ) // NOLINT(bugprone-empty-catch)
        {
            // we can safely ignore,
            // the data probably just didn't include the NEXT marker
        }
        if( !next_magic.empty() && next_magic != k_next_magic )
        {
            throw std::runtime_error{
                fmt::format("CDRM NEXT magic invalid, found \"{}\", wanted \"{}\"", next_magic, k_next_magic)};
        }
        
        return out_data;
    }
}
