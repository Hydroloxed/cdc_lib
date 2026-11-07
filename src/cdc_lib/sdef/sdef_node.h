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
        std::string name{};
        sdef_type_ref type{};
    };

    struct sdef_enum
    {
        std::string name{};
        sdef_primitive_type underlying_type{};
    };

    struct sdef_struct
    {
        std::string name{};
    };

    struct sdef_module
    {
        std::string name{};
        std::string cpp_namespace{};
    };

    struct sdef_node
    {
        sdef_node* parent{nullptr};
        std::variant< sdef_module,
                      sdef_enum,
                      sdef_struct,
                      sdef_var > data{};
        std::vector< std::unique_ptr< sdef_node > > children{};

        explicit sdef_node( sdef_node* a_parent ) : parent( a_parent ) {}

        [[nodiscard]] bool is_module() const noexcept { return std::holds_alternative< sdef_module >( data ); }
        [[nodiscard]] sdef_module& as_module() { return std::get< sdef_module >( data ); }
        [[nodiscard]] const sdef_module& as_module() const { return std::get< sdef_module >( data ); }

        [[nodiscard]] bool is_struct() const noexcept { return std::holds_alternative< sdef_struct >( data ); }
        [[nodiscard]] sdef_struct& as_struct() { return std::get< sdef_struct >( data ); }
        [[nodiscard]] const sdef_struct& as_struct() const { return std::get< sdef_struct >( data ); }

        [[nodiscard]] bool is_enum() const noexcept { return std::holds_alternative< sdef_enum >( data ); }
        [[nodiscard]] sdef_enum& as_enum() { return std::get< sdef_enum >( data ); }
        [[nodiscard]] const sdef_enum& as_enum() const { return std::get< sdef_enum >( data ); }

        [[nodiscard]] bool is_var() const noexcept { return std::holds_alternative< sdef_var >( data ); }
        [[nodiscard]] sdef_var& as_var() { return std::get< sdef_var >( data ); }
        [[nodiscard]] const sdef_var& as_var() const { return std::get< sdef_var >( data ); }

        sdef_node* find_type(std::string_view a_name)
        {
            for( auto& child : children )
            {
                if( child->is_enum() && child->as_enum().name == a_name )
                    return &*child;
            }
            if( parent )
                return parent->find_type( a_name );
            return nullptr;
        }
    };
}

#endif
