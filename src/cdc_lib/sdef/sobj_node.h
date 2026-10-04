#ifndef CDC_LIB_SDEF_SOBJ_NODE_H
#define CDC_LIB_SDEF_SOBJ_NODE_H
#include <cstdint>
#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace cdc_lib::sdef
{
    struct sdef_node;

    struct sobj_var
    {
        std::variant<bool, float, double, std::intmax_t, std::uintmax_t, std::string> data{};

        [[nodiscard]] bool is_bool() const noexcept { return std::holds_alternative<bool>(data); }
        [[nodiscard]] bool is_float32() const noexcept { return std::holds_alternative<float>(data); }
        [[nodiscard]] bool is_float64() const noexcept { return std::holds_alternative<double>(data); }
        [[nodiscard]] bool is_int() const noexcept { return std::holds_alternative<std::intmax_t>(data); }
        [[nodiscard]] bool is_uint() const noexcept { return std::holds_alternative<std::uintmax_t>(data); }
        [[nodiscard]] bool is_string() const noexcept { return std::holds_alternative<std::string>(data); }

        [[nodiscard]] bool as_bool() const { return std::get<bool>(data); }
        [[nodiscard]] float as_float32() const { return std::get<float>(data); }
        [[nodiscard]] double as_float64() const { return std::get<double>(data); }
        [[nodiscard]] std::intmax_t as_int() const { return std::get<std::intmax_t>(data); }
        [[nodiscard]] std::uintmax_t as_uint() const { return std::get<std::uintmax_t>(data); }
        [[nodiscard]] const std::string& as_string() const { return std::get<std::string>(data); }
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
