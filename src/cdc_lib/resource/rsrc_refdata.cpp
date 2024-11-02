#include "rsrc_refdata.h"
#include <cassert>
#include <cdc_lib/resource/rsrc_resolve_object.h>
#include <score/binary_io/binio_strings.h>
#include <string_view>

namespace cdc_lib::resource::ref_data
{
    namespace
    {
        void read( score::binary_io::input_interface& a_input, cooked_resource_guid& a_guid )
        {
            a_guid.type = static_cast< cooked_resolve_section_type >( read< std::uint32_t >( a_input ) );
            a_guid.id = read< std::uint32_t >( a_input );
        }

        void write( score::binary_io::output_interface& a_output, const cooked_resource_guid& a_guid )
        {
            write( a_output, static_cast< std::uint32_t >( a_guid.type ) );
            write( a_output, a_guid.id );
        }

        void read( score::binary_io::input_interface& a_input, section_ref_data& a_section )
        {
            read( a_input, a_section.offset );
            read( a_input, a_section.id );
            read( a_input, a_section.resource_type );
            a_section.section_type = static_cast< cooked_resolve_section_type >( read< std::uint32_t >( a_input ) );
            read( a_input, a_section.size );
            read( a_input, a_section.relocation_table_size );
        }

        void write( score::binary_io::output_interface& a_output, const section_ref_data& a_section )
        {
            write( a_output, a_section.offset );
            write( a_output, a_section.id );
            write( a_output, a_section.resource_type );
            write( a_output, static_cast< std::uint32_t >( a_section.section_type ) );
            write( a_output, a_section.size );
            write( a_output, a_section.relocation_table_size );
        }

        void read( score::binary_io::input_interface& a_input, object_ref_data& a_object )
        {
            a_object.offset = read< std::uint64_t >( a_input );
            a_object.primary_section = read< std::uint64_t >( a_input );
            a_object.path_hash = read< std::uint64_t >( a_input );
            a_object.path = read_c_string( a_input );
            const auto includes_size = read< std::uint32_t >( a_input );
            a_object.includes.reserve( includes_size );
            for( auto i = 0u; i < includes_size; ++i )
                a_object.includes.push_back( read_c_string( a_input ) );
            const auto depends_size = read< std::uint32_t >( a_input );
            a_object.objects_that_depend_on.reserve( depends_size );
            for( auto i = 0u; i < depends_size; ++i )
                a_object.objects_that_depend_on.push_back( read_c_string( a_input ) );
            const auto references_size = read< std::uint32_t >( a_input );
            a_object.references_sections.reserve( references_size );
            for( auto i = 0u; i < references_size; ++i )
            {
                cooked_resource_guid guid{};
                read( a_input, guid );
                a_object.references_sections.push_back( guid );
            }
        }

        void write( score::binary_io::output_interface& a_output, const object_ref_data& a_object )
        {
            write( a_output, a_object.offset );
            write( a_output, a_object.primary_section );
            write( a_output, a_object.path_hash );
            a_output.write( std::as_bytes( std::span{a_object.path} ) );
            write( a_output, '\0' );
            write( a_output, static_cast< std::uint32_t >( a_object.includes.size() ) );
            for( const auto& include : a_object.includes )
            {
                a_output.write( std::as_bytes( std::span{include} ) );
                write( a_output, '\0' );
            }
            write( a_output, static_cast< std::uint32_t >( a_object.objects_that_depend_on.size() ) );
            for( const auto& depends : a_object.objects_that_depend_on )
            {
                a_output.write( std::as_bytes( std::span{depends} ) );
                write( a_output, '\0' );
            }
            write( a_output, static_cast< std::uint32_t >( a_object.references_sections.size() ) );
            for( const auto& guid : a_object.references_sections )
                write( a_output, guid );
        }

        constexpr std::string_view k_magic = "REFD";
    }

    void save_refdata( score::binary_io::output_interface& a_output, const ref_data& a_ref_data )
    {
        a_output.write( std::as_bytes( std::span{k_magic} ) );
        write( a_output, static_cast< std::uint64_t >( a_ref_data.sections.size() ) );
        for( const auto& [guid, section] : a_ref_data.sections )
        {
            write( a_output, guid );
            write( a_output, section );
        }
        write( a_output, static_cast< std::uint64_t >( a_ref_data.objects.size() ) );
        for( const auto& object : a_ref_data.objects )
            write( a_output, object );
    }

    ref_data load_refdata( score::binary_io::input_interface& a_input )
    {
        std::string magic = read_fixed_string( a_input, k_magic.size() );
        if( magic != k_magic )
            throw std::runtime_error{"Invalid magic"};

        ref_data ret{};
        const auto section_count = read< std::uint64_t >( a_input );
        for( std::uint64_t i = 0; i < section_count; ++i )
        {
            cooked_resource_guid guid{};
            read( a_input, guid );
            assert( ret.sections.find( guid ) == ret.sections.end() );
            if( ret.sections.find( guid ) != ret.sections.end() )
                throw std::runtime_error{"Duplicate section GUID detected. This is a bug!"};
            read( a_input, ret.sections[guid] );
        }
        const auto object_count = read< std::uint64_t >( a_input );
        for( std::uint64_t i = 0; i < object_count; ++i )
        {
            object_ref_data object{};
            read( a_input, object );
            ret.objects.push_back( object );
        }
        // This part is really. really. slow.
        // As in, it takes around half a second (of 0.8 seconds) on TRAS bigfile.tiger on my machine compiled with debug.
        // Buuuuuuuut..... we need this data anyway, so we can't skip this.
        // TODO: at least make this optional since it's not needed for every usecase.
        for( auto& object : ret.objects )
        {
            for( const auto& guid : object.references_sections )
            {
                assert( ret.sections.find( guid ) != ret.sections.end() );
                auto& section = ret.sections.at( guid );
                section.referenced_by_objects.push_back( &object );
            }
        }

        return ret;
    }
}
