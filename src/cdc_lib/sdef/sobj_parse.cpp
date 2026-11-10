#include "sobj_parse.h"
#include <cassert>
#include <cdc_lib/resource/rsrc_reloc_input_stream.h>
#include <cdc_lib/sdef/sdef_node.h>
#include <cdc_lib/sdef/sdef_type.h>
#include <cdc_lib/sdef/sobj_node.h>
#include <score/binary_io/binio_strings.h>

namespace cdc_lib::sdef
{
    namespace
    {
        std::string read_string( resource::reloc_istream& a_data )
        {
            const auto scope = resource::reloc_istream_scope{a_data, "sdef.string"};
            if( !scope )
                return {};
            return score::binary_io::read_c_string( a_data );
        }

        sobj_prim_data read_primitive( resource::reloc_istream& a_data,
                                       sdef_primitive_type a_prim_type )
        {
            using enum sdef_primitive_type;

            switch( a_prim_type )
            {
                case none: assert(false && "Compound types not supported"); break;
                // TODO: Should we check/warn if we read something other
                //       than 1 or 0? That would be a good chance to catch
                //       possible errors in SDEF definitions...
                // clang-format off
                case bool8:   return bool{read< std::uint8_t >(a_data) != 0};
                case float32: return read< float >(a_data);
                case float64: return read< double >(a_data);
                case uint8:   return std::uintmax_t{read< std::uint8_t >(a_data)};
                case uint16:  return std::uintmax_t{read< std::uint16_t >(a_data)};
                case uint32:  return std::uintmax_t{read< std::uint32_t >(a_data)};
                case uint64:  return std::uintmax_t{read< std::uint64_t >(a_data)};
                case int8:    return read< std::int8_t >(a_data);
                case int16:   return read< std::int16_t >(a_data);
                case int32:   return read< std::int32_t >(a_data);
                case int64:   return read< std::int64_t >(a_data);
                case string:  return read_string( a_data );
                // clang-format on
                default: assert(false && "Unsupported primitive type. Bug in sobj::read_primitive"); break;
            }
        }

        sobj_node parse_sobj_impl( sdef_node* a_sdef,
                                   sobj_node* a_parent,
                                   resource::reloc_istream& a_data );
        
        void read_var( sobj_node& a_node,
                       sdef_node* a_sdef,
                       resource::reloc_istream& a_data )
        {
            const auto& var = a_sdef->as_var();
            if( var.type.primitive_type == sdef_primitive_type::none )
            {
                sdef_node* type = var.type.type;
                assert( type );
                if( type->is_enum() )
                {
                    a_node.var_data.data = read_primitive( a_data, type->as_enum().underlying_type );
                }
                else if( type->is_struct() )
                {
                    a_node = parse_sobj_impl( type, &a_node, a_data);
                    // Set the sdef pointer to the variable instead of the struct
                    a_node.sdef = a_sdef;
                }
                else
                {
                    assert( false && "Only enums and structs are implemented for now" );
                }
            }
            else
            {
                a_node.var_data.data = read_primitive( a_data, var.type.primitive_type );
            }
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
                for( const auto& child : a_sdef->children )
                {
                    if( child->is_var() )
                    {
                        node.children.push_back( std::make_unique< sobj_node >(
                            parse_sobj_impl( child.get(), &node, a_data ) ) );
                    }
                }
            }
            else if( a_sdef->is_var() )
            {
                read_var( node, a_sdef, a_data );
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
