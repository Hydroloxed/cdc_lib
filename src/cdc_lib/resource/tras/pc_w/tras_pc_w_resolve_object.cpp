#include "tras_pc_w_resolve_object.h"
#include <cassert>
#include <cdc_lib/resource/rsrc_resolve_object.h>
#include <cstdint>
#include <score/binary_io/binio_strings.h>
#include <score/containers/simple_lookup_table.h>
#include <score/score_bit.h>


namespace cdc_lib::resource::tras::pc_w
{
    namespace
    {
        constexpr std::uint32_t k_version = 0x16;

        constexpr std::uint32_t k_no_primary_section = static_cast< std::uint32_t >( -1 );

        [[maybe_unused]] constexpr std::uint32_t k_flag_section_padding = 0x1;

        constexpr auto k_section_info_bit_range_has_debug_info = score::bit_range{0, 1};
        constexpr auto k_section_info_bit_range_resource_type = score::bit_range{1, 7};
        constexpr auto k_section_info_bit_range_reloc_table_size = score::bit_range{8, 24};
        constexpr auto k_section_extra_info_bit_range_unique_id_id = score::bit_range{0, 24};
        constexpr auto k_section_extra_info_bit_range_unique_id_type = score::bit_range{24, 8};

        constexpr score::simple_lookup_table< std::uint8_t, cooked_resolve_section_type, 11 > k_section_type_lookup =
        {
            std::pair{0, cooked_resolve_section_type::general},
            std::pair{2, cooked_resolve_section_type::animation},
            std::pair{5, cooked_resolve_section_type::texture},
            std::pair{6, cooked_resolve_section_type::wave},
            std::pair{7, cooked_resolve_section_type::dtp},
            std::pair{8, cooked_resolve_section_type::script},
            std::pair{9, cooked_resolve_section_type::shader},
            std::pair{10, cooked_resolve_section_type::material},
            std::pair{11, cooked_resolve_section_type::object},
            std::pair{12, cooked_resolve_section_type::render_resource},
            std::pair{13, cooked_resolve_section_type::collision_mesh}
        };

        void read( score::binary_io::input_interface& a_input_interface, cooked_resolve_section_type& a_section_type )
        {
            a_section_type = k_section_type_lookup.lookup( read< std::uint8_t >( a_input_interface ) );
        }

        std::vector< std::string > read_string_list( score::binary_io::input_interface& a_input_interface,
                                                     std::size_t a_byte_length )
        {
            const auto start = a_input_interface.tell();
            std::vector< std::string > strings;
            while( a_input_interface.tell() - start < a_byte_length )
                strings.emplace_back( read_c_string( a_input_interface ) );
            return strings;
        }
    }

    std::unique_ptr< cooked_resolve_object > load_object( score::binary_io::input_interface& a_input_interface )
    {
        auto object = std::make_unique< cooked_resolve_object >();

        [[maybe_unused]] auto version = read< std::uint32_t >( a_input_interface );
        assert( version == k_version ); // TODO: throw exception
        const auto include_length  = read< std::uint32_t >( a_input_interface );
        const auto depends_length  = read< std::uint32_t >( a_input_interface );
        [[maybe_unused]] const auto padding_length  = read< std::uint32_t >( a_input_interface );
        assert( padding_length == 0 ); // not yet supported
        [[maybe_unused]] const auto projected_size  = read< std::uint32_t >( a_input_interface );
        // assert( projected_size == 0 ); - TODO(logging): this can actually happen
        // we should probably encourage the user to report this somewhere!
        [[maybe_unused]] const auto flags           = read< std::uint32_t >( a_input_interface );
        assert( flags == 0 ); // also not known to happen with non-debug data
        const auto section_count   = read< std::uint32_t >( a_input_interface );
        assert( section_count < (1u << 16u) ); // maximum number of sections supported by the game
        const auto primary_section = read< std::uint32_t >( a_input_interface );
        assert( primary_section < section_count || primary_section == k_no_primary_section );

        for( std::uint32_t i = 0; i < section_count; i++ )
        {
            auto& section = object->sections.emplace_back();
            section.size = read< std::uint32_t >( a_input_interface );
            read( a_input_interface, section.type );

            // we aren't really interested in these flags
            // (SectionInfo::skip, SectionInfo::tigerPatch)
            [[maybe_unused]] const auto misc_flags = read< std::uint8_t >( a_input_interface );

            section.version_id = read< std::uint16_t >( a_input_interface );
            const auto packed = read< std::uint32_t >( a_input_interface );
            section.has_debug_info = score::get_bits( packed, k_section_info_bit_range_has_debug_info ) != 0u;
            section.resource_type = score::get_bits( packed, k_section_info_bit_range_resource_type );
            section.relocation_table_size = score::get_bits( packed, k_section_info_bit_range_reloc_table_size );
            section.id = read< std::uint32_t >( a_input_interface );
            section.spec_mask = read< std::uint32_t >( a_input_interface );
        }

        object->primary_section = primary_section == k_no_primary_section
                                ? nullptr : &object->sections[ primary_section ];
        auto strings = read_string_list( a_input_interface, include_length + depends_length );
        for( std::size_t pos = 0; const auto& string : strings )
        {
            if( pos < include_length )
                object->includes.emplace_back( string );
            else
                object->objects_that_depend_on.emplace_back( string );
            pos += string.size() + 1;
        }

        for( auto& section : object->sections )
        {
            const auto unique_id = read< std::uint32_t >( a_input_interface );
            const auto unique_id_id = score::get_bits( unique_id, k_section_extra_info_bit_range_unique_id_id );
            [[maybe_unused]] const auto unique_id_type = score::get_bits( unique_id, k_section_extra_info_bit_range_unique_id_type );
            if( section.id == 0 )
                section.id = unique_id_id;
            else
            {
                assert( section.id == unique_id_id );
                assert( section.type == k_section_type_lookup.lookup( unique_id_type ) );
            }
            section.extra_data.packed_offset = read< std::uint32_t >( a_input_interface );
            section.extra_data.compressed_size = read< std::uint32_t >( a_input_interface );
            section.extra_data.decompressed_offset = read< std::uint32_t >( a_input_interface );
        }

        return object;
    }

    void write_object( score::binary_io::output_interface& a_output,
                       const cooked_resolve_object& a_object,
                       const std::vector< std::string >& a_section_datas )
    {
        const auto total_strvec_size = []( const std::vector< std::string >& a_vec )
        {
            std::size_t size = 0;
            for( const auto& str : a_vec )
                size += str.size() + 1; // nullterm
            return size;
        };

        write< std::uint32_t >( a_output, k_version );
        write< std::uint32_t >( a_output, total_strvec_size( a_object.includes ) );
        write< std::uint32_t >( a_output, total_strvec_size( a_object.objects_that_depend_on ) );
        write< std::uint32_t >( a_output, 0u ); // padding
        write< std::uint32_t >( a_output, 0u ); // projected size
        write< std::uint32_t >( a_output, 0u ); // flags
        write< std::uint32_t >( a_output, a_object.sections.size() );
        if( a_object.primary_section )
            write( a_output, static_cast< std::uint32_t >( a_object.primary_section - a_object.sections.data() ) );
        else
            write( a_output, static_cast< std::uint32_t >( k_no_primary_section ) );
        for( const auto& section : a_object.sections )
        {
            write< std::uint32_t >( a_output, section.size );
            write< std::uint8_t >( a_output, k_section_type_lookup.lookup_key( section.type ) );
            write< std::uint8_t >( a_output, 0u ); // misc flags
            write< std::uint16_t >( a_output, section.version_id );
            std::uint32_t packed = 0u;
            packed = score::set_bits( packed, k_section_info_bit_range_has_debug_info, section.has_debug_info ? 1u : 0u );
            packed = score::set_bits( packed, k_section_info_bit_range_resource_type, static_cast< std::uint32_t >( section.resource_type ) );
            packed = score::set_bits( packed, k_section_info_bit_range_reloc_table_size, static_cast< std::uint32_t >( section.relocation_table_size ) );
            write< std::uint32_t >( a_output, packed );
            write< std::uint32_t >( a_output, section.id );
            write< std::uint32_t >( a_output, section.spec_mask );
        }
        for( const auto& str : a_object.includes )
        {
            for( const auto& ch : str )
                write< std::uint8_t >( a_output, ch );
            write< std::uint8_t >( a_output, 0u );
        }
        for( const auto& str : a_object.objects_that_depend_on )
        {
            for( const auto& ch : str )
                write< std::uint8_t >( a_output, ch );
            write< std::uint8_t >( a_output, 0u );
        }
        int i = 0;
        for( const auto& data : a_section_datas )
        {
            i = i + 1;
            const auto& section = a_object.sections[i - 1];
            const auto interface = score::binary_io::create_input_interface( data );
            if( section.relocation_table_size )
            {
                std::uint32_t sizes[5]{};
                for( auto& s : sizes )
                    s = read< std::uint32_t >( *interface );
                const auto rsize = sizes[0] * 8 + sizes[1] * 4 + sizes[2] * 8 + sizes[3] * 4 + sizes[4] * 4 + 0x14;
                if( section.relocation_table_size != rsize )
                {
                    std::printf("reloc table size mismatch: %x data vs %zx SectionInfo\n", rsize, section.relocation_table_size );
                    std::printf("in section %d (%d type %d)\n", i - 1, section.id, (int)section.type);
                }
            }
            if( section.relocation_table_size + section.size != data.size() )
                std::printf( "size mismatch\n" );
            a_output.write( std::as_bytes( std::span{data} ) );
        }
    }
}
