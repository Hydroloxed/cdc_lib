#ifndef CDC_LIB_RESOURCE_TRAS_PC_W_TRAS_PC_W_RESOURCE_H
#define CDC_LIB_RESOURCE_TRAS_PC_W_TRAS_PC_W_RESOURCE_H
#include <cstdint>

namespace cdc_lib::resource::tras::pc_w
{
    class resource_ref_id
    {
        constexpr static std::uint64_t k_resource_id_shift = 0;
        constexpr static std::uint64_t k_resource_id_mask = UINT32_MAX;
        constexpr static std::uint64_t k_section_type_shift = 32;
        constexpr static std::uint64_t k_section_type_mask = (std::uint64_t) UINT8_MAX << k_section_type_shift;
    public:
        resource_ref_id( std::uint64_t a_packed ) :
            resource_id( (a_packed & k_resource_id_mask) >> k_resource_id_shift ),
            section_type( (a_packed & k_section_type_mask) >> k_section_type_shift )
        {}
        resource_ref_id( std::uint32_t a_resource_id, std::uint8_t a_section_type ) :
            resource_id( a_resource_id ),
            section_type( a_section_type )
        {}
        std::uint64_t pack() const noexcept
        {
            std::uint64_t ret{0};
            ret |= (resource_id & k_resource_id_mask) << k_resource_id_shift;
            ret |= (section_type & k_section_type_mask) << k_section_type_shift;
            return ret;
        }

        std::uint32_t resource_id{0};
        std::uint8_t  section_type{0};
    };

    inline resource_ref_id unpack_section_guid( std::uint32_t a_packed_guid )
    {
        constexpr auto k_resource_id_mask  = ((1 << 25) - 1) << 0;
        constexpr auto k_section_type_shift = 25;
        constexpr auto k_section_type_mask = ((1 << 7) - 1) << 25;
        const auto resource_id  = a_packed_guid & k_resource_id_mask;
        const auto section_type = (a_packed_guid & k_section_type_mask) >> k_section_type_shift;
        return resource_ref_id{resource_id, static_cast< std::uint8_t >( section_type ) };
    }
}

#endif
