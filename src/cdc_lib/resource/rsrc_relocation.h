#ifndef CDC_LIB_RESOURCE_RSRC_RELOCATION_H
#define CDC_LIB_RESOURCE_RSRC_RELOCATION_H
#include "rsrc_resource.h"
#include <optional>

namespace cdc_lib::resource
{
    struct cooked_relocation
    {
        unsigned src_ptr_offset{0};
        unsigned dest_ptr_offset{0};
        std::optional< resource_ref > resource{};

        [[nodiscard]] bool is_internal() const noexcept { return !resource.has_value(); }
        [[nodiscard]] bool is_external() const noexcept { return resource.has_value(); }
    };
}

#endif
