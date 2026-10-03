#include "sobj_parse.h"
#include <cdc_lib/sdef/sdef_node.h>
#include <score/binary_io/binio_strings.h>

namespace cdc_lib::sdef
{
    namespace
    {
        std::string read_string( resource::reloc_istream& a_data )
        {
            auto scope = resource::reloc_istream_scope{a_data, "sdef.string"};
            if( !scope )
                return {};
            return score::binary_io::read_c_string( a_data );
        }

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
                    // TODO: Should we check/warn if we read something other
                    //       than 1 or 0? That would be a good chance to catch
                    //       possible errors in SDEF definitions...
                    // clang-format off
                    case bool8:   node.var_data.data = bool{read< std::uint8_t >(a_data) != 0}; break;
                    case float32: node.var_data.data = read< float >(a_data); break;
                    case float64: node.var_data.data = read< double >(a_data); break;
                    case uint8:   node.var_data.data = std::uintmax_t{read< std::uint8_t >(a_data)}; break;
                    case uint16:  node.var_data.data = std::uintmax_t{read< std::uint16_t >(a_data)}; break;
                    case uint32:  node.var_data.data = std::uintmax_t{read< std::uint32_t >(a_data)}; break;
                    case uint64:  node.var_data.data = std::uintmax_t{read< std::uint64_t >(a_data)}; break;
                    case int8:    node.var_data.data = read< std::int8_t >(a_data); break;
                    case int16:   node.var_data.data = read< std::int16_t >(a_data); break;
                    case int32:   node.var_data.data = read< std::int32_t >(a_data); break;
                    case int64:   node.var_data.data = read< std::int64_t >(a_data); break;
                    case string:  node.var_data.data = read_string( a_data ); break;
                    // clang-format on
                    default: assert(false && "Unsupported primitive type. Bug in parse_sobj"); break;
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
