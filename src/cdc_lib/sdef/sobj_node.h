#ifndef CDC_LIB_SDEF_SOBJ_NODE_H
#define CDC_LIB_SDEF_SOBJ_NODE_H
#include <cstdint>
#include <memory>
#include <variant>
#include <vector>

namespace cdc_lib::sdef
{
    struct sdef_node;

    struct sobj_var
    {
        // TODO: For now we only support ints...
        std::variant<bool, std::intmax_t, std::uintmax_t> data{};

        bool is_bool() const noexcept { return std::holds_alternative<bool>(data); }
        bool is_int() const noexcept { return std::holds_alternative<std::intmax_t>(data); }
        bool is_uint() const noexcept { return std::holds_alternative<std::uintmax_t>(data); }

        bool as_bool() const { return std::get<bool>(data); }
        std::intmax_t as_int() const { return std::get<std::intmax_t>(data); }
        std::uintmax_t as_uint() const { return std::get<std::uintmax_t>(data); }
    };

    struct sobj_node
    {
        sdef_node* sdef{nullptr}; //< Corresponding node in definition tree
        sobj_node* parent{nullptr};
        sobj_var var_data{};
        std::vector< std::unique_ptr< sobj_node > > children{};
    };
}

#endif
