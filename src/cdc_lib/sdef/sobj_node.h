#ifndef CDC_LIB_SDEF_SOBJ_NODE_H
#define CDC_LIB_SDEF_SOBJ_NODE_H
#include <memory>
#include <vector>

namespace cdc_lib::sdef
{
    struct sdef_node;

    struct sobj_var
    {
        // TODO: For now we only support ints...
        // This should also be variant<intmax_t, uintmax_t>!
        std::intmax_t data{};
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
