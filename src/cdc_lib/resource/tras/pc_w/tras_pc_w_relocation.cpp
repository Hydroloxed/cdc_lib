#include "tras_pc_w_relocation.h"
#include "tras_pc_w_resource.h"
#include <cassert>
#include <cstdint>

namespace cdc_lib::resource::tras::pc_w
{
    namespace
    {
        constexpr std::uint32_t k_resource_pointer_resource_id_offset_mask = (1u << 25u) - 1u;
        [[maybe_unused]] constexpr std::uint32_t k_resource_pointer_resource_id_offset_shift = 0;
        constexpr std::uint32_t k_resource_pointer_resource_type_shift = 25;
        [[maybe_unused]] constexpr std::uint32_t k_resource_pointer_resource_type_mask = ((1u << 7u) - 1u) << k_resource_pointer_resource_type_shift;
    }

    [[nodiscard]] std::vector< cooked_relocation > load_relocation_table( score::binary_io::input_interface& a_input_interface )
    {
        const auto intern_ptr_count       = read< std::uint32_t >( a_input_interface );
        const auto extern_ptr_count       = read< std::uint32_t >( a_input_interface );
        const auto resource_id_count      = read< std::uint32_t >( a_input_interface );
        const auto resource_id_16_count   = read< std::uint32_t >( a_input_interface );
        const auto resource_pointer_count = read< std::uint32_t >( a_input_interface );
        // We've already read the header, so we don't need to add the size of it here.
        const auto relocation_table_size  = intern_ptr_count * 8 +
                                            extern_ptr_count * 8 +
                                            resource_id_count * 4 +
                                            resource_id_16_count * 8 +
                                            resource_pointer_count * 4;
        const auto data_offset = a_input_interface.tell() + relocation_table_size;

        std::vector< cooked_relocation > relocations;
        relocations.reserve( intern_ptr_count +
                             extern_ptr_count +
                             resource_id_count +
                             resource_id_16_count +
                             resource_pointer_count );

        auto read_intern_ptr = [&a_input_interface]()
        {
            auto ptr_offset = read< std::uint32_t >( a_input_interface );
            auto referenced_offset = read< std::uint32_t >( a_input_interface );
            return cooked_relocation
            {
                .src_ptr_offset = ptr_offset,
                .dest_ptr_offset = referenced_offset,
                .resource = std::nullopt
            };
        };
        auto read_resource_pointer = [&a_input_interface, data_offset]()
        {
            const auto packed = read< std::uint32_t >( a_input_interface );
            const auto resource_guid_offset = (packed & k_resource_pointer_resource_id_offset_mask) * 4;
            // This is unnecessary, we get this from the resource guid.
            const auto resource_type = (packed & k_resource_pointer_resource_type_mask) >> k_resource_pointer_resource_type_shift;
            const auto resource_id = [&a_input_interface, data_offset, resource_guid_offset]()
            {
                const auto old_stream_position = a_input_interface.tell();
                a_input_interface.seek( data_offset + resource_guid_offset );
                const auto read_val = read< std::uint32_t >( a_input_interface );
                a_input_interface.seek( old_stream_position );
                return read_val;
            }();
            return cooked_relocation
            {
                .src_ptr_offset = resource_guid_offset,
                .dest_ptr_offset = 0,
                .resource = resource_ref{ resource_ref_id{ resource_id, static_cast< std::uint8_t >( resource_type ) }.pack() }
            };
        };
        for( std::uint32_t i = 0; i < intern_ptr_count; i++ )
            relocations.push_back( read_intern_ptr() );
        // TODO: extern_ptrs
        assert( extern_ptr_count == 0 );
        // TODO: resource_id_ptrs
        assert( resource_id_count == 0 );
        // TODO: resource_id_16_ptrs
        assert( resource_id_16_count == 0 );
        for( std::uint32_t i = 0; i < resource_pointer_count; i++ )
            relocations.push_back( read_resource_pointer() );
        return relocations;
    }
}
