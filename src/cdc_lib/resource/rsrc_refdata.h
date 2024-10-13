#ifndef CDC_LIB_RESOURCE_RSRC_REFDATA_H
#define CDC_LIB_RESOURCE_RSRC_REFDATA_H
#include <cdc_lib/resource/rsrc_resolve_object.h>
#include <cstdint>
#include <deque>
#include <score/binary_io/binary_io.h>
#include <string>
#include <unordered_map>
#include <vector>

namespace cdc_lib::resource::ref_data
{
    struct object_ref_data;

    struct section_ref_data
    {
        std::uint64_t offset{};
        std::uint64_t size{};
        std::uint64_t relocation_table_size{};
        std::uint32_t id{};
        std::uint8_t resource_type{};
        cooked_resolve_section_type section_type{};
        std::vector< object_ref_data* > referenced_by_objects{};

        [[nodiscard]] cooked_resource_guid guid() const noexcept { return {section_type, id}; }
    };

    struct object_ref_data
    {
        std::uint64_t offset{};
        std::uint64_t primary_section{};
        std::uint64_t path_hash{};
        std::string path{};
        std::vector< std::string > includes;
        std::vector< std::string > objects_that_depend_on{};
        std::vector< cooked_resource_guid > references_sections{};
    };

    struct ref_data
    {
        std::deque< object_ref_data > objects{};
        // unordered_map is 3x faster than map here
        // (2,93 s to load with map, 0,96 s with unordered_map)
        std::unordered_map< cooked_resource_guid, section_ref_data > sections{};
    };

    void save_refdata( score::binary_io::output_interface& a_output, const ref_data& a_ref_data );
    ref_data load_refdata( score::binary_io::input_interface& a_input );
}

#endif
