#include "tras_pc_w_relocation.h"
#include "tras_pc_w_resource.h"
#include <cassert>
#include <cdc_lib/resource/rsrc_relocation.h>
#include <cstdint>
#include <score/score_bit.h>

namespace cdc_lib::resource::tras::pc_w
{
    namespace
    {
        constexpr score::bit_range k_resource_guid_id_range   = {0u,  25u};
        constexpr score::bit_range k_resource_guid_type_range = {25u, 7u};

        
        constexpr score::bit_range k_resource_pointer_offset_range   = {0u, 25u};
        constexpr score::bit_range k_resource_pointer_type_range = {25u, 7u};

        constexpr score::bit_range k_extern_ptr_section_index_range [[maybe_unused]] = {0u,  16u};
        constexpr score::bit_range k_extern_ptr_pointer_offset_range                 = {16u, 23u};
        constexpr score::bit_range k_extern_ptr_referenced_offset_range              = {39u, 25u};

        // There's technically no maximum size for the relocation table, but
        // this is the largest possible value that can be stored in SectionInfo.
        constexpr std::uint32_t k_max_reloc_table_size = (1u << 24u) - 1u; // INT24_MAX
    }

    [[nodiscard]] std::vector< cooked_relocation > load_relocation_table( score::binary_io::input_interface& a_input_interface )
    {
        const auto intern_ptr_count              = read< std::uint32_t >( a_input_interface );
        const auto extern_ptr_count              = read< std::uint32_t >( a_input_interface );
        const auto resource_id_count             = read< std::uint32_t >( a_input_interface );
        const auto resource_id_16_count          = read< std::uint32_t >( a_input_interface );
        const auto resource_pointer_count        = read< std::uint32_t >( a_input_interface );
        const std::size_t relocation_table_size  = intern_ptr_count * 8u +
                                                   extern_ptr_count * 8u +
                                                   resource_id_count * 4u +
                                                   resource_id_16_count * 8u +
                                                   resource_pointer_count * 4u;
        if( relocation_table_size > k_max_reloc_table_size )
            throw relocation_table_invalid{"Reloc table WAAAY to large (>INT24_MAX)."
                                           "Verify that you are using the correct endian and platform."};
        // We've already read the header, so we don't need to add the size of it here.
        const auto data_offset = a_input_interface.tell() + relocation_table_size;

        std::vector< cooked_relocation > relocations;
        relocations.reserve( intern_ptr_count +
                             extern_ptr_count +
                             resource_id_count +
                             resource_id_16_count +
                             resource_pointer_count );

        const auto read_intern_ptr = [&a_input_interface]()
        {
            const auto ptr_offset = read< std::uint32_t >( a_input_interface );
            const auto referenced_offset = read< std::uint32_t >( a_input_interface );
            return cooked_relocation
            {
                .src_ptr_offset = ptr_offset,
                .dest_ptr_offset = referenced_offset,
                .resource = std::nullopt,
                .type = cooked_relocation_type::internal
            };
        };
        const auto read_extern_ptr = [&a_input_interface, data_offset]()
        {
            const auto packed = read< std::uint64_t >( a_input_interface );
            const auto pointer_offset = score::get_bits( packed, k_extern_ptr_pointer_offset_range ) * 4;
            const auto packed_resource_guid = [&a_input_interface, pointer_offset, data_offset]()
            {
                const auto old_stream_position = a_input_interface.tell();
                a_input_interface.seek( data_offset + pointer_offset );
                const auto read_val = read< std::uint32_t >( a_input_interface );
                a_input_interface.seek( old_stream_position );
                return read_val;
            }();
            // NOTE: The GUID can be any uint32, not just a id + type.
            // TODO: The resource should really be looked up in the DRM/DB.
            //       So, provide a callback parameter like the game's ResolveTypeAndId.
            const auto resource_id = score::get_bits( packed_resource_guid, k_resource_guid_id_range );
            const auto resource_type = score::get_bits( packed_resource_guid, k_resource_guid_type_range );
            return cooked_relocation
            {
                .src_ptr_offset = static_cast< unsigned >( pointer_offset ),
                .dest_ptr_offset = static_cast< unsigned >( score::get_bits( packed, k_extern_ptr_referenced_offset_range ) ),
                .resource = resource_ref{resource_ref_id{resource_id, static_cast< std::uint8_t >( resource_type )}.pack()},
                .type = cooked_relocation_type::external
            };
        };
        const auto read_resource_id_16 = [&a_input_interface, data_offset]()
        {
            constexpr score::bit_range k_resource_type_range     = {0u, 16u};
            // only half of the resource id is stored here; the other half is at *offset*
            // crystal is definitely messing with us here...
            constexpr score::bit_range k_resource_id_high_range = {16u, 16u};
            const auto packed = read< std::uint32_t >( a_input_interface );
            const auto resource_type = score::get_bits( packed, k_resource_type_range );
            const auto resource_id_high = score::get_bits( packed, k_resource_id_high_range );
            const auto resource_id_low_offset = read< std::uint32_t >( a_input_interface );
            const auto resource_id_low = [&a_input_interface, data_offset, resource_id_low_offset]()
            {
                const auto old_stream_position = a_input_interface.tell();
                a_input_interface.seek( data_offset + resource_id_low_offset );
                const auto read_val = read< std::uint32_t >( a_input_interface );
                a_input_interface.seek( old_stream_position );
                return read_val;
            }();
            // This is assert is valid only if we're assuming `resource_id == rtr_id`...
            // But code (https://sourcevc.intra.hydrolox.me/Hydrolox/tras_src/src/branch/main/cdc/runtime/cdcResource/TigerSectionLoader.cpp#L648)
            // uses FindResource() here, and the whole point of ResourceID16 is
            // to fit a resource id > 16 bits into a RTRID of 16 bits...
            // Ugh... Whatever, this reloc type aren't used anyway...
            assert( resource_id_high == 0 ); 
            const auto resource_id = (resource_id_high << 16u) | resource_id_low;
            return cooked_relocation
            {
                .src_ptr_offset = static_cast< unsigned >( resource_id_low_offset ),
                .dest_ptr_offset = static_cast< unsigned >( resource_id_high ),
                .resource = resource_ref{resource_ref_id{resource_id, static_cast< std::uint8_t >( resource_type )}.pack()},
                .type = cooked_relocation_type::resource_id16
            };
        };
        const auto read_resource_pointer = [&a_input_interface, data_offset]( bool a_is_resource_id )
        {
            // resource pointer ids are always 31 bits
            // the top bit is used to express whether the pointer is optional
            // if so, it will be set to null if resource id wasn't found at runtime
            // otherwise, it will assert
            // Source: (https://sourcevc.intra.hydrolox.me/Hydrolox/tras_src/src/branch/main/cdc/runtime/cdcResource/TigerSectionLoader.cpp#L673-L687)
            constexpr std::uint32_t k_resource_id_mask = 0x7fffffffu;
            [[maybe_unused]] constexpr std::uint32_t k_resource_id_unknown_bit_mask = 0x80000000u;
            const auto packed = read< std::uint32_t >( a_input_interface );
            const auto resource_id_offset = score::get_bits( packed, k_resource_pointer_offset_range ) * 4u;
            const auto resource_type = score::get_bits( packed, k_resource_pointer_type_range );
            const auto resource_id = [&a_input_interface, data_offset, resource_id_offset]()
            {
                const auto old_stream_position = a_input_interface.tell();
                a_input_interface.seek( data_offset + resource_id_offset );
                const auto read_val = read< std::uint32_t >( a_input_interface );
                a_input_interface.seek( old_stream_position );
                return read_val & k_resource_id_mask;
            }();
            return cooked_relocation
            {
                .src_ptr_offset = resource_id_offset,
                .dest_ptr_offset = 0,
                .resource = resource_ref{ resource_ref_id{ resource_id, static_cast< std::uint8_t >( resource_type ) }.pack() },
                .type = a_is_resource_id ? cooked_relocation_type::resource_id : cooked_relocation_type::resource_pointer
            };
        };
        for( std::uint32_t i = 0; i < intern_ptr_count; i++ )
            relocations.push_back( read_intern_ptr() );
        for( std::uint32_t i = 0; i < extern_ptr_count; i++ )
            relocations.push_back( read_extern_ptr() );
        // identical to resource_id relocations, except for one tiny detail
        // TODO: investigate how these are chosen when building resources
        for( std::uint32_t i = 0; i < resource_id_count; i++ )
            relocations.push_back( read_resource_pointer( true ) );
        for( std::uint32_t i = 0; i < resource_id_16_count; i++ )
            relocations.push_back( read_resource_id_16() );
        for( std::uint32_t i = 0; i < resource_pointer_count; i++ )
            relocations.push_back( read_resource_pointer( false ) );
        assert( a_input_interface.tell() == data_offset );
        return relocations;
    }

    void write_relocation_table( score::binary_io::output_interface& a_output_interface,
                                 std::vector< cooked_relocation > a_relocations,
                                 std::string& a_resource_data )
    {
        std::vector< cooked_relocation > intern_ptrs{};
        std::vector< cooked_relocation > extern_ptrs{};
        std::vector< cooked_relocation > resource_ptrs{};
        for( const auto& relocation : a_relocations )
        {
            if( relocation.is_internal() )
                intern_ptrs.push_back( relocation );
            else if( relocation.type == cooked_relocation_type::external )
                extern_ptrs.push_back( relocation );
            else if( relocation.type == cooked_relocation_type::resource_pointer )
                resource_ptrs.push_back( relocation );
            else
                assert( false && "Unsupported relocation" );
        }
        write< std::uint32_t >( a_output_interface, intern_ptrs.size() );
        write< std::uint32_t >( a_output_interface, extern_ptrs.size() );
        write< std::uint32_t >( a_output_interface, 0ul );
        write< std::uint32_t >( a_output_interface, 0ul );
        write< std::uint32_t >( a_output_interface, resource_ptrs.size() );
        for( const auto& ptr : intern_ptrs )
        {
            write< std::uint32_t >( a_output_interface, ptr.src_ptr_offset );
            write< std::uint32_t >( a_output_interface, ptr.dest_ptr_offset );
        }
        for( const auto& ptr : extern_ptrs )
        {
            std::uint64_t packed = 0;
            packed = score::set_bits< std::uint64_t >( packed, k_extern_ptr_pointer_offset_range, ptr.src_ptr_offset / 4 ); 
            packed = score::set_bits< std::uint64_t >( packed, k_extern_ptr_referenced_offset_range, ptr.dest_ptr_offset );
            write< std::uint64_t >( a_output_interface, packed );
            std::uint32_t guid = 0;
            const resource_ref_id id{ptr.resource->get_user_id()};
            guid = score::set_bits< std::uint32_t >( guid, k_resource_guid_id_range, id.resource_id );
            guid = score::set_bits< std::uint32_t >( guid, k_resource_guid_type_range, id.section_type );
            std::string temp_data{};
            auto temp_oo = score::binary_io::create_output_interface( temp_data, a_output_interface.endian() );
            write< std::uint32_t >( *temp_oo, guid );
            a_resource_data[ptr.src_ptr_offset] = temp_data[0];
            a_resource_data[ptr.src_ptr_offset + 1] = temp_data[1];
            a_resource_data[ptr.src_ptr_offset + 2] = temp_data[2];
            a_resource_data[ptr.src_ptr_offset + 3] = temp_data[3];
        }
        for( const auto& ptr : resource_ptrs )
        {
            const resource_ref_id id{ptr.resource->get_user_id()};
            std::uint32_t packed = 0;
            packed = score::set_bits< std::uint32_t >( packed, k_resource_pointer_offset_range, ptr.src_ptr_offset / 4 );
            packed = score::set_bits< std::uint32_t >( packed, k_resource_pointer_type_range, id.section_type );
            write< std::uint32_t >( a_output_interface, packed );

            std::string temp_data{};
            auto temp_oo = score::binary_io::create_output_interface( temp_data, a_output_interface.endian() );
            write< std::uint32_t >( *temp_oo, id.resource_id );
            a_resource_data[ptr.src_ptr_offset] = temp_data[0];
            a_resource_data[ptr.src_ptr_offset + 1] = temp_data[1];
            a_resource_data[ptr.src_ptr_offset + 2] = temp_data[2];
            a_resource_data[ptr.src_ptr_offset + 3] = temp_data[3];
        }
    }
}
