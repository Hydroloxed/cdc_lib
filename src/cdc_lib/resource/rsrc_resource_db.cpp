#include "rsrc_resource_db.h"
#include <algorithm>
#include <cassert>
#include <cdc_lib/file/tras/pc_w/tras_pc_w_compression.h>
#include <fmt/core.h>
#include <score/binary_io/binary_io.h>
#include <score/binary_io/binio_strings.h>
#include <score/score_bit.h>

namespace cdc_lib::resource
{
    namespace
    {
        constexpr std::string_view k_magic = "ResourceDB";

        constexpr score::bit_range k_guid_id_range = {0u, 25u};
        constexpr score::bit_range k_guid_type_range = {25u, 7u};
        constexpr score::bit_range k_reloc_table_size_range = {0, 25u};
        constexpr score::bit_range k_resource_type_range = {25u, 7u};
        constexpr std::uint32_t k_offset_64_bit_flag = 1u << 31u;
        // Version history:
        // - 1: initial version
        constexpr std::uint32_t k_version = 1u;

        void write( score::binary_io::output_interface& a_output, cooked_resource_guid a_guid )
        {
            std::uint32_t packed{0};
            packed = score::set_bits( packed, k_guid_id_range, a_guid.id );
            packed = score::set_bits( packed, k_guid_type_range, static_cast< std::uint32_t >( a_guid.type ) );
            write( a_output, packed );
        }

        void read( score::binary_io::input_interface& a_input, cooked_resource_guid& a_guid )
        {
            const auto packed = read< std::uint32_t >( a_input );
            a_guid.id = score::get_bits( packed, k_guid_id_range );
            a_guid.type = static_cast< cooked_resolve_section_type >( score::get_bits( packed, k_guid_type_range ) );
        }

        void write( score::binary_io::output_interface& a_output, const cooked_resolve_section& a_resource )
        {
            assert(  a_resource.version_id == 0 );
            assert( !a_resource.has_debug_info );
            // pack reloc size and resource type into 4 bytes, instead of using 9 bytes
            {
                const auto reloc_table_size = static_cast< std::uint32_t >( a_resource.relocation_table_size );
                std::uint32_t packed = 0;
                packed = score::set_bits( packed, k_reloc_table_size_range, reloc_table_size );
                packed = score::set_bits( packed, k_resource_type_range, static_cast< std::uint32_t >( a_resource.resource_type ) );
                write( a_output, packed );
            }
            write( a_output, static_cast< std::uint32_t >( a_resource.size ) );
            write( a_output, static_cast< std::uint32_t >( a_resource.extra_data.compressed_size ) ); 
            // write( a_output, a_resource.get_section_metadata().extra_data.decompressed_offset ); - we will never use this
            const auto packed_offset = a_resource.extra_data.packed_offset;
            // save a few bytes by using a 32 bit offset, if possible
            // use the fact that bigfiles are never larger than 2 GiB, and hence the top bit will never be set
            if( packed_offset > k_offset_64_bit_flag )
            {
                assert( (packed_offset & k_offset_64_bit_flag) == 0u );
                write( a_output, static_cast< std::uint32_t >( packed_offset | k_offset_64_bit_flag ) );
                write( a_output, static_cast< std::uint32_t >( packed_offset >> 32u ) );
            }
            else
                write( a_output, static_cast< std::uint32_t >( packed_offset ) );
        }

        resource read_resource( score::binary_io::input_interface& a_input, cooked_resource_guid a_guid )
        {
            cooked_resolve_section section_metadata{};
            {
                const auto packed = read< std::uint32_t >( a_input );
                section_metadata.resource_type = score::get_bits( packed, k_resource_type_range );
                section_metadata.relocation_table_size = score::get_bits( packed, k_reloc_table_size_range );
            }
            section_metadata.size = read< std::uint32_t >( a_input );
            section_metadata.extra_data.compressed_size = read< std::uint32_t >( a_input );
            const auto packed_offset = read< std::uint32_t >( a_input );
            section_metadata.extra_data.packed_offset = packed_offset & ~k_offset_64_bit_flag;
            if( packed_offset & k_offset_64_bit_flag )
                section_metadata.extra_data.packed_offset |= std::uint64_t{read< std::uint32_t >( a_input )} << 32u;
            section_metadata.id = a_guid.id;
            section_metadata.type = a_guid.type;
            return resource{section_metadata};
        }

        void write( score::binary_io::output_interface& a_output, const db_object& a_object )
        {
            write( a_output, static_cast< std::uint16_t >( a_object.get_path().size() ) );
            a_output.write( std::as_bytes( std::span{a_object.get_path()} ) );
            write( a_output, a_object.get_primary_resource_ref() );
            write( a_output, static_cast< std::uint32_t >( a_object.iterate_resource_references().size() ) );
            for( const auto& ref : a_object.iterate_resource_references() )
                write( a_output, ref );
        }

        db_object read_db_object( score::binary_io::input_interface& a_input )
        {
            auto path = score::binary_io::read_fixed_string( a_input, read< std::uint16_t >( a_input ) );
            cooked_resource_guid primary_resource_ref{};
            read( a_input, primary_resource_ref );
            db_object object{path};
            object.set_primary_resource_ref( primary_resource_ref );
            const auto ref_count = read< std::uint32_t >( a_input );
            for( std::uint32_t i = 0u; i < ref_count; ++i )
            {
                cooked_resource_guid ref{};
                read( a_input, ref );
                object.add_resource_ref( ref );
            }
            return object;
        }
    }

    resource_db load_db( score::binary_io::input_interface& a_input )
    {
        resource_db db;
        std::string magic = score::binary_io::read_fixed_string( a_input, k_magic.size() );
        if( magic != k_magic )
            throw std::runtime_error{fmt::format( R"(Invalid magic, found '{}', expected '{}')", magic, k_magic )};
        const auto version = score::binary_io::read< std::uint32_t >( a_input );
        if( version != k_version )
            throw std::runtime_error{fmt::format( "Invalid version, found {}, expected {}", version, k_version )};
        const auto resource_count = score::binary_io::read< std::uint32_t >( a_input );
        const auto object_count = score::binary_io::read< std::uint32_t >( a_input );
        for( std::uint64_t i = 0u; i < resource_count; ++i )
        {
            cooked_resource_guid guid{};
            read( a_input, guid );
            db.insert( guid, read_resource( a_input, guid ) );
        }
        for( std::uint64_t i = 0u; i < object_count; ++i )
            db.insert( read_db_object( a_input ) );
        return db;
    }

    void save_db( score::binary_io::output_interface& a_output, const resource_db& a_db )
    {
        a_output.write( std::as_bytes( std::span{k_magic} ) );
        write( a_output, k_version );
        write( a_output, static_cast< std::uint32_t >( a_db.size() ) );
        write( a_output, static_cast< std::uint32_t >( a_db.objects_size() ) );
        for( const auto& [guid, resource] : a_db )
        {
            write( a_output, guid );
            write( a_output, resource.get_section_metadata() );
        }
        for( const auto& object : a_db.iterate_objects() )
            write( a_output, object.second );
    }

    std::span< const db_object* const > resource::get_referenced_by_objects() const noexcept
    {
        return std::span{referenced_by_objects};
    }

    void resource::load_sync()
    {
        assert( db );
        auto data = db->load_offset_sync( section_metadata.extra_data.packed_offset, section_metadata.extra_data.compressed_size );
        auto ii = score::binary_io::create_input_interface( data );
        if( file::tras::pc_w::is_cdrm( *ii ) )
            data = file::tras::pc_w::decompress_cdrm( *ii );
        raw_data = data;
    }

    bool db_object::is_loaded() const
    {
        assert( db );
        return std::ranges::all_of( referenced_resources, [this]( cooked_resource_guid a_resource ) { return db->at( a_resource ).is_loaded(); } );
    }
	
	void resource_db::add_resource_references()
	{
		for( auto& [name, o] : db_objects )
		{
			for( const auto& r : o.iterate_resource_references() )
				at( r ).referenced_by_objects.push_back( &o );
		}
	}

    resource* db_object::get_primary_resource() { return &db->at( primary_section ); }

    std::optional< std::size_t > db_object::find_section_index( cooked_resource_guid a_guid ) const
    {
        for( std::size_t i = 0u; i < referenced_resources.size(); ++i )
            if( referenced_resources[i] == a_guid )
                return i;
        return std::nullopt;
    }

    std::string resource_db::load_offset_sync( std::uint64_t a_offset, std::size_t a_size )
    {
        return file::read_offset( archive_fs, a_offset, a_size );
    }

    void resource_db::insert( cooked_resource_guid a_guid, resource&& a_resource )
    {
        assert( (a_resource.db == nullptr || a_resource.db == this)
            && "Trying to transfer a resource from one db to another. This will open up all sorts of issues that I'm not ready to deal with" );
        a_resource.db = this;
        resources[a_guid] = a_resource;
    }

    void resource_db::insert( std::string_view a_path, db_object&& a_object )
    {
        assert( a_object.db == nullptr );
        a_object.db = this;
        db_objects.emplace( a_path, std::move( a_object ) );
    }
}
