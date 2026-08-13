#include "sdef_parse.h"
#include <algorithm>
#include <cassert>
#include <cctype>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tinyxml2.h>

namespace cdc_lib::sdef
{
    namespace
    {
        bool check_type( tinyxml2::XMLElement* a_xml_node, std::string_view a_name )
        {
            assert( a_xml_node );

            const auto comp_ci = []( char a_a, char a_b ) -> bool
            {
                return std::tolower( static_cast< unsigned char >( a_a ) )
                    == std::tolower( static_cast< unsigned char >( a_b ) );
            };

            const auto name = std::string{a_xml_node->Name()};
            return std::ranges::equal( name, a_name, comp_ci );
        }

        std::optional< std::string> get_string_attr( tinyxml2::XMLElement* a_xml_node,
                                                     const char* a_name )
        {
            assert( a_xml_node );

            const char* value = nullptr;
            if( a_xml_node->QueryStringAttribute( a_name, &value ) == tinyxml2::XML_SUCCESS )
                return value;
            return std::nullopt;
        }

        sdef_primitive_type get_primitive_type( std::string_view a_type )
        {
            if( a_type == "bool8" ) return sdef_primitive_type::bool8;
            if( a_type == "int8" ) return sdef_primitive_type::int8;
            if( a_type == "uint8" ) return sdef_primitive_type::uint8;
            if( a_type == "int16" ) return sdef_primitive_type::int16;
            if( a_type == "uint16" ) return sdef_primitive_type::uint16;
            if( a_type == "int32" ) return sdef_primitive_type::int32;
            if( a_type == "uint32" ) return sdef_primitive_type::uint32;
            if( a_type == "int64" ) return sdef_primitive_type::int64;
            if( a_type == "uint64" ) return sdef_primitive_type::uint64;
            if( a_type == "float32" ) return sdef_primitive_type::float32;
            return sdef_primitive_type::none;
        }

        void do_subtree( tinyxml2::XMLElement* a_xml_node, sdef_node& a_node )
        {
            if( check_type( a_xml_node, "struct" ) )
            {
                a_node.data = sdef_struct{};
                a_node.as_struct().name = get_string_attr( a_xml_node, "name" ).value();
            }
            else if( check_type( a_xml_node, "var" ) )
            {
                a_node.data = sdef_var{};
                a_node.as_var().name = get_string_attr( a_xml_node, "name" ).value();
                const auto type = sdef_type_ref{get_string_attr( a_xml_node, "type" ).value()};
                a_node.as_var().type = type;
                a_node.as_var().type.primitive_type = get_primitive_type( a_node.as_var().type.type_name );
            }
            for( auto* child = a_xml_node->FirstChildElement();
                 child;
                 child = child->NextSiblingElement() )
            {
                a_node.children.push_back( std::make_unique< sdef_node >( &a_node ) );
                auto& new_node = *a_node.children.back();
                do_subtree( child, new_node );
            }
        }
    }

    sdef_node parse_sdef( const std::string& a_sdef_file_data )
    {
        tinyxml2::XMLDocument doc;
        doc.Parse( a_sdef_file_data.c_str() );
        sdef_node root{nullptr};
        if( doc.FirstChildElement() == nullptr
         || doc.FirstChildElement()->NextSiblingElement() != nullptr )
            throw std::runtime_error{"You must have exactly one root element"};
        do_subtree( doc.FirstChildElement(), root );
        return root;
    }
}
