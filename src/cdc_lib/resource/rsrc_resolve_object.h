#ifndef CDC_LIB_RESOURCE_RSRC_RESOLVE_OBJECT_H
#define CDC_LIB_RESOURCE_RSRC_RESOLVE_OBJECT_H
#include <any>
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

    struct cooked_resolve_section
    {
        std::size_t size{0};
        std::size_t relocation_table_size{0};
        cooked_resolve_section_type type{cooked_resolve_section_type::unknown};
        std::uint16_t version_id{0};
        std::uint32_t id{0};
        bool has_debug_info{false};
        std::uint8_t resource_type{0};
        // TODO: specialization masks
        std::any extra_data{};
    };

    struct cooked_resolve_object
    {
        std::string path{"<unknown path>"};
        std::vector< cooked_resolve_section > sections{};
        std::vector< std::string > includes{};
        std::vector< std::string > objects_that_depend_on{};
        cooked_resolve_section* primary_section{nullptr};
    };
}

#endif
