#include "sobj_parse.h"
#include <cdc_lib/sdef/sdef_node.h>

namespace cdc_lib::sdef
{
    namespace
    {
        sobj_node parse_sobj_impl( sdef_node* a_sdef,
                                   sobj_node* a_parent,
                                   resource::reloc_istream& a_data )
        {
            sobj_node node{};
            node.sdef = a_sdef;
            node.parent = a_parent;
            if( a_sdef->is_struct() )
            {
                for( auto& child : a_sdef->children )
                {
                    node.children.push_back( std::make_unique< sobj_node >(
                        parse_sobj_impl( child.get(), &node, a_data ) ) );
                }
            }
            else if( a_sdef->is_var() )
            {
                auto& var = a_sdef->as_var();
                using enum sdef_primitive_type;
                assert( var.type.primitive_type != none );
                switch( var.type.primitive_type )
                {
                    case none: assert(false && "Compound types not supported"); break;
                    // TODO: Should support all the types we listed in sdef_primitive_type
                    case uint8: node.var_data.data = read< std::uint8_t >(a_data); break;
                    default: assert(false && "Unsupported primitive type"); break;
                }
            }
            return node;
        }
    }

    sobj_node parse_sobj( sdef_node* a_root_struct,
                          resource::reloc_istream& a_data )
    {
        return parse_sobj_impl( a_root_struct, nullptr, a_data );
    }
}
