#include "tras_pc_w_resolve_object.h"
#include "cdc_lib/resource/rsrc_resolve_object.h"
#include <cassert>
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

        constexpr auto k_section_info_bit_range_has_debug_info = score::bit_range{ 0, 1 };
        constexpr auto k_section_info_bit_range_resource_type = score::bit_range{ 1, 7 };
        constexpr auto k_section_info_bit_range_reloc_table_size = score::bit_range{ 8, 24 };

        constexpr score::simple_lookup_table< std::uint8_t, cooked_resolve_section_type, 11 > section_type_lookup =
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
            a_section_type = section_type_lookup.lookup( read< std::uint8_t >( a_input_interface ) );
        }

        std::vector< std::string > read_string_list( score::binary_io::input_interface& a_input_interface,
                                                     std::size_t a_byte_length )
        {
            const auto start = a_input_interface.tell();
            std::vector< std::string > strings;
            while( a_input_interface.tell() - start < a_byte_length )
                read_c_string( a_input_interface, strings.emplace_back() );
            return strings;
        }
    }

    std::unique_ptr< cooked_resolve_object > load_object( score::binary_io::input_interface& a_input_interface )
    {
        auto object = std::make_unique< cooked_resolve_object >();

        auto version = read< std::uint32_t >( a_input_interface );
        assert( version == k_version ); // TODO: throw exception
        const auto include_length  = read< std::uint32_t >( a_input_interface );
        const auto depends_length  = read< std::uint32_t >( a_input_interface );
        const auto padding_length  = read< std::uint32_t >( a_input_interface );
        assert( padding_length == 0 ); // not yet supported
        [[maybe_unused]] const auto projected_size  = read< std::uint32_t >( a_input_interface );
        // assert( projected_size == 0 ); - TODO(logging): this can actually happen
        // we should probably encourage the user to report this somewhere!
        const auto flags           = read< std::uint32_t >( a_input_interface );
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
            [[maybe_unused]] const auto spec_mask = read< std::uint32_t >( a_input_interface );
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
            auto& ex = section.extra_data.emplace< cooked_resolve_section_extra_data >();
            read( a_input_interface, ex.unique_id );
            read( a_input_interface, ex.packed_offset );
            read( a_input_interface, ex.compressed_size );
            read( a_input_interface, ex.decompressed_offset );
        }

        return object;
    }
}
