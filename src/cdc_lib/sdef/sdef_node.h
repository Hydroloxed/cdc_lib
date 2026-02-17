#ifndef CDC_LIB_SDEF_SDEF_NODE_H
#define CDC_LIB_SDEF_SDEF_NODE_H
#include <cdc_lib/sdef/sdef_type.h>
#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace cdc_lib::sdef
{
    struct sdef_var
    {
        sdef_type_ref type{};
    };

    struct sdef_struct
    {
        std::string name{};
    };

    struct sdef_node
    {
        sdef_node* parent{nullptr};
        std::variant< sdef_struct > data{};
        std::vector< std::unique_ptr< sdef_node > > children{};

        explicit sdef_node( sdef_node* a_parent ) : parent( a_parent ) {}

        [[nodiscard]] bool is_struct() const noexcept { return std::holds_alternative< sdef_struct >( data ); }
        [[nodiscard]] sdef_struct& as_struct() { return std::get< sdef_struct >( data ); }
        [[nodiscard]] const sdef_struct& as_struct() const { return std::get< sdef_struct >( data ); }
    };
}

#endif
