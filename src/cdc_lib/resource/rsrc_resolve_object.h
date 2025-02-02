#ifndef CDC_LIB_RESOURCE_RSRC_RESOLVE_OBJECT_H
#define CDC_LIB_RESOURCE_RSRC_RESOLVE_OBJECT_H
#include <cstdint>
#include <string>
#include <vector>

namespace cdc_lib::resource
{
    enum class cooked_resolve_section_type
    {
        unknown,
        general,
        animation,
        texture,
        wave,
        dtp,
        script,
        shader,
        material,
        object,
        render_resource,
        collision_mesh
    };

    struct cooked_resource_guid
    {
        cooked_resolve_section_type type{};
        std::uint32_t id{};

        auto operator<=>( const cooked_resource_guid& ) const = default;
    };

    struct cooked_resolve_section
    {
        struct extra_info
        {
            std::uint64_t packed_offset{0};
            std::size_t compressed_size{0};
            std::uint32_t decompressed_offset{0};
        };

        std::size_t size{0};
        std::size_t relocation_table_size{0};
        cooked_resolve_section_type type{cooked_resolve_section_type::unknown};
        std::uint16_t version_id{0};
        std::uint32_t id{0};
        bool has_debug_info{false};
        std::uint8_t resource_type{0};
        extra_info extra_data{};
        // TODO: specialization masks

        [[nodiscard]] cooked_resource_guid guid() const { return {type, id}; }
    };

    struct cooked_resolve_object
    {
        std::string path{};
        std::vector< cooked_resolve_section > sections{};
        std::vector< std::string > includes{};
        std::vector< std::string > objects_that_depend_on{};
        cooked_resolve_section* primary_section{nullptr};

        [[nodiscard]] bool has_path() const { return !path.empty(); }
    };
}

template<>
struct std::hash< cdc_lib::resource::cooked_resource_guid >
{
    std::size_t operator()( const cdc_lib::resource::cooked_resource_guid& a_guid ) const
    {
        const auto h1 = std::hash< cdc_lib::resource::cooked_resolve_section_type >{}( a_guid.type );
        const auto h2 = std::hash< std::uint32_t >{}( a_guid.id );
        return h1 ^ (h2 << 3u);
    }
};

#endif
