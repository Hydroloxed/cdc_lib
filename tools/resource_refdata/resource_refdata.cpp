#include <cassert>
#include <cdc_lib/core/core_crc32.h>
#include <cdc_lib/file/archive_fs.h>
#include <cdc_lib/file/tras/pc_w/tras_pc_w_compression.h>
#include <cdc_lib/file/tras/pc_w/tras_pc_w_tiger.h>
#include <cdc_lib/resource/rsrc_refdata.h>
#include <cdc_lib/resource/rsrc_resolve_object.h>
#include <cdc_lib/resource/tras/pc_w/tras_pc_w_resolve_object.h>
#include <cstdlib>
#include <cxxopts.hpp>
#include <fmt/core.h>
#include <fmt/ostream.h>
#include <fstream>
#include <ranges>
#include <score/binary_io/binary_io.h>
#include <score/containers/simple_lookup_table.h>

namespace
{
    [[nodiscard]] std::string to_string( cdc_lib::resource::cooked_resolve_section_type a_section_type )
    {
        switch( a_section_type )
        {
            case cdc_lib::resource::cooked_resolve_section_type::unknown: return "unknown";
            case cdc_lib::resource::cooked_resolve_section_type::general: return "general";
            case cdc_lib::resource::cooked_resolve_section_type::animation: return "animation";
            case cdc_lib::resource::cooked_resolve_section_type::texture: return "texture";
            case cdc_lib::resource::cooked_resolve_section_type::wave: return "wave";
            case cdc_lib::resource::cooked_resolve_section_type::dtp: return "dtp";
            case cdc_lib::resource::cooked_resolve_section_type::script: return "script";
            case cdc_lib::resource::cooked_resolve_section_type::shader: return "shader";
            case cdc_lib::resource::cooked_resolve_section_type::material: return "material";
            case cdc_lib::resource::cooked_resolve_section_type::object: return "object";
            case cdc_lib::resource::cooked_resolve_section_type::render_resource: return "render_resource";
            case cdc_lib::resource::cooked_resolve_section_type::collision_mesh: return "collision_mesh";
        }
        assert( false );
    }
    [[nodiscard]] cdc_lib::resource::cooked_resolve_section_type
        string_to_section_type( const std::string& a_string )
    {
        if( a_string == "general" || a_string == "gnrl" )
            return cdc_lib::resource::cooked_resolve_section_type::general;
        if( a_string == "animation" || a_string == "anim" )
            return cdc_lib::resource::cooked_resolve_section_type::animation;
        if( a_string == "texture" || a_string == "tex" )
            return cdc_lib::resource::cooked_resolve_section_type::texture;
        if( a_string == "wave" || a_string == "wav" || a_string == "sound" || a_string == "snd" )
            return cdc_lib::resource::cooked_resolve_section_type::wave;
        if( a_string == "dtp" )
            return cdc_lib::resource::cooked_resolve_section_type::dtp;
        if( a_string == "script" || a_string == "scrt" )
            return cdc_lib::resource::cooked_resolve_section_type::script;
        if( a_string == "shader" || a_string == "shad" || a_string == "shdr" )
            return cdc_lib::resource::cooked_resolve_section_type::shader;
        if( a_string == "material" || a_string == "mat" || a_string == "matd" )
            return cdc_lib::resource::cooked_resolve_section_type::material;
        if( a_string == "object" || a_string == "obj" || a_string == "objt" )
            return cdc_lib::resource::cooked_resolve_section_type::object;
        if( a_string == "render_resource" || a_string == "mesh" || a_string == "terrain" || a_string == "rndr" )
            return cdc_lib::resource::cooked_resolve_section_type::render_resource;
        if( a_string == "collision_mesh" || a_string == "collide" || a_string == "cmsh" )
            return cdc_lib::resource::cooked_resolve_section_type::collision_mesh;
        fmt::print( "unknown section type {}\n", a_string );
        fmt::print( "valid section types (case-sensitive, with shorthands):\n" );
        fmt::print( "general, gnrl\n" );
        fmt::print( "animation, anim\n" );
        fmt::print( "texture, tex\n" );
        fmt::print( "wave, wav, sound, snd\n" );
        fmt::print( "dtp\n" );
        fmt::print( "script, scrt\n" );
        fmt::print( "shader, shad, shdr\n" );
        fmt::print( "material, mat, matd\n" );
        fmt::print( "object, obj, objt\n" );
        fmt::print( "render_resource, mesh, terrain, rndr\n" );
        fmt::print( "collision_mesh, collide, cmsh\n" );
        std::exit( EXIT_FAILURE ); // NOLINT(concurrency-mt-unsafe)
        return cdc_lib::resource::cooked_resolve_section_type::unknown;
    }

    cdc_lib::resource::ref_data::ref_data load_refdata( std::string_view a_filename )
    {
        auto interface = score::binary_io::open_file( a_filename );
        return cdc_lib::resource::ref_data::load_refdata( *interface );
    }
}

namespace operation_create
{
    void    add_object_references( cdc_lib::resource::ref_data::ref_data& a_global_ref_data,
                                   cdc_lib::resource::ref_data::object_ref_data& a_object_ref_data,
                                   const cdc_lib::resource::cooked_resolve_object& a_resolve_object )
    {
        auto find_or_create_section = [&]( const cdc_lib::resource::cooked_resolve_section& a_section )
        {
            auto found = a_global_ref_data.sections.find( {a_section.type, a_section.id} );
            if( found != a_global_ref_data.sections.end() )
            {
                a_object_ref_data.references_sections.push_back( {a_section.type, a_section.id} );
                found->second.referenced_by_objects.push_back( &a_object_ref_data );
            }
            else
            {
                cdc_lib::resource::ref_data::section_ref_data section_ref_data{};
                section_ref_data.section_type = a_section.type;
                section_ref_data.resource_type = a_section.resource_type;
                section_ref_data.id = a_section.id;
                section_ref_data.offset = a_section.extra_data.packed_offset;
                section_ref_data.compressed_size = a_section.extra_data.compressed_size;
                section_ref_data.size = a_section.size;
                section_ref_data.relocation_table_size = a_section.relocation_table_size;
                a_global_ref_data.sections.insert(
                    std::pair{section_ref_data.guid(), section_ref_data} );
                a_object_ref_data.references_sections.push_back( {a_section.type, a_section.id} );
            }
        };

        for( const auto& section : a_resolve_object.sections )
            find_or_create_section( section );
    }

    cdc_lib::resource::ref_data::ref_data create_references( const cdc_lib::file::archive& a_archive,
                                                             const std::map< std::uint32_t, std::string >& a_hash_to_path )
    {
        cdc_lib::resource::ref_data::ref_data ref_data{};
        auto is_resolve_object = []( std::string_view a_data )
        {
            constexpr std::size_t k_resolve_object_min_size = 0x20;
            return a_data.size() >= k_resolve_object_min_size &&
                   a_data[0] == '\x16' && a_data[1] == '\x00' && a_data[2] == '\x00' && a_data[3] == '\x00';
        };
        for( const auto& record : a_archive.records )
        {
            auto data = cdc_lib::file::tras::pc_w::read_record( a_archive, record );
            if( !is_resolve_object( data ) )
                continue;

            try
            {
                auto input_interface = score::binary_io::create_input_interface( data, std::endian::little );
                auto resolve_object = cdc_lib::resource::tras::pc_w::load_object( *input_interface );
                cdc_lib::resource::ref_data::object_ref_data object_ref_data{};
                add_object_references( ref_data, object_ref_data, *resolve_object );
                object_ref_data.offset = record.offset;
                object_ref_data.path_hash = record.name_hash;
                object_ref_data.path = a_hash_to_path.contains( record.name_hash ) ?
                    a_hash_to_path.at( record.name_hash ) :
                    fmt::format( "{:08x}", record.name_hash );
                object_ref_data.includes = resolve_object->includes;
                object_ref_data.objects_that_depend_on = resolve_object->objects_that_depend_on;
                ref_data.objects.push_back( object_ref_data );
            }
            catch( std::exception& )
            {
                fmt::print( "Error reading object {:16x}\n", record.name_hash );
            }
        }
        return ref_data;
    }

    std::map< std::uint32_t, std::string > make_hash_list( std::ifstream&& a_stream )
    {
        if( !a_stream.good() )
            return {};
        // a_stream.exceptions( std::ios::badbit | std::ios::eofbit | std::ios::failbit );
        std::map< std::uint32_t, std::string > result;
        std::string path{};
        while( std::getline( a_stream, path ) )
        {
            const auto hash = cdc_lib::core::crc32( path );
            result[hash] = path;
        }
        return result;
    }

    int run( int a_argc, char** a_argv )
    {
        cxxopts::Options options( "resource_refdata", "Create a resource refdata" );
        options.add_options()
            ( "operation", "The operation to perform", cxxopts::value< std::string >() )
            ( "l,filelist", "The file containing list of files in the archive", cxxopts::value< std::string >() )
            ( "t,tiger", "The tiger files to read data from", cxxopts::value< std::string >() )
            ( "o,output", "The output file", cxxopts::value< std::string >()->default_value( "refdata.bin" ) )
            ( "h,help", "Print help" );
        options.parse_positional( {"operation", "tiger"} );
        options.positional_help( "<tiger files>" );
        auto result = options.parse( a_argc, a_argv );
        if( result.count( "help" ) != 0 )
        {
            fmt::print( "{}\n", options.help() );
            return EXIT_FAILURE;
        }
        const auto& filename = result["tiger"].as< std::string >();
        std::ifstream stream{filename};
        if( !stream.good() )
        {
            fmt::print( stderr, "Could not open file '{}'\n", filename.c_str() );
            return EXIT_FAILURE;
        }
        stream.exceptions( std::ios::badbit | std::ios::eofbit | std::ios::failbit );
        std::map< std::uint32_t, std::string > hash_to_path{};
        if( result.count( "filelist" ) != 0 )
            hash_to_path = make_hash_list( std::ifstream{result["filelist"].as< std::string >()} );
        auto input_interface = score::binary_io::create_input_interface( stream, std::endian::little );
        auto archive = cdc_lib::file::tras::pc_w::load_archive( *input_interface, filename );
        auto refdata = create_references( archive, hash_to_path );
        fmt::print( "Created refdata for archive \"{}\"\n", filename );
        std::ofstream output{result["output"].as< std::string >()};
        output.exceptions( std::ios::badbit | std::ios::eofbit | std::ios::failbit );
        auto output_interface = score::binary_io::create_output_interface( output, std::endian::little );
        cdc_lib::resource::ref_data::save_refdata( *output_interface, refdata );
        return EXIT_SUCCESS;
    }
}

namespace operation_find
{
    enum class comparison
    {
        equal,
        not_equal,
        less_than,
        greater_than,
    };

    enum class filter_by
    {
        none,
        id,
        resource_type,
        section_type,
        refs,
        size,
        reloc_table_size
    };

    struct filter
    {
        filter_by by{};
        comparison comparison{};
        std::uint32_t value{};
    };

    template< std::three_way_comparable value_type >
    bool perform_op( value_type a_lhs, value_type a_rhs, comparison a_comparison )
    {
        switch( a_comparison )
        {
            case comparison::equal:
                return a_lhs == a_rhs;
            case comparison::not_equal:
                return a_lhs != a_rhs;
            case comparison::less_than:
                return a_lhs < a_rhs;
            case comparison::greater_than:
                return a_lhs > a_rhs;
        }
    }

    constexpr score::simple_lookup_table< std::string, filter_by, 6 > k_filter_by_lookup =
    {
        std::pair{ "id", filter_by::id },
        std::pair{ "rt", filter_by::resource_type },
        std::pair{ "st", filter_by::section_type },
        std::pair{ "refs", filter_by::refs },
        std::pair{ "s", filter_by::size },
        std::pair{ "rs", filter_by::reloc_table_size }
    };

    /// parse filters of the form "rt=5, id=0x12345678"
    std::vector< filter > parse_filter( std::string_view a_filter )
    {
        std::vector< filter > filters;
        for( auto part : a_filter | std::views::split( ',' ) )
        {
            std::string filter_by_str{};
            std::string comparison_str{};
            std::string value_str{};
            for( auto c : part )
            {
                const std::string comparison_chars{ "=!<>" };
                // read filter_by
                if( comparison_str.empty() && comparison_chars.find( c ) == std::string::npos )
                    filter_by_str += c;
                // read comparison
                else if( comparison_chars.find( c ) != std::string::npos )
                    comparison_str += c;
                // read value
                else
                    value_str += c;
            }
            filter_by filter_by = k_filter_by_lookup.lookup_or( filter_by_str, filter_by::none );
            score::simple_lookup_table< std::string, comparison, 4 > k_comparison_lookup =
            {
                std::pair{ "=", comparison::equal },
                std::pair{ "!=", comparison::not_equal },
                std::pair{ "<", comparison::less_than },
                std::pair{ ">", comparison::greater_than }
            };
            comparison comparison = k_comparison_lookup.lookup_or( comparison_str, comparison::equal );

            std::uint32_t value = [value_str, filter_by]()
            {
                if( value_str.starts_with( "0x" ) )
                    return std::stoul( value_str.substr( 2 ), nullptr, 16 ); // NOLINT(readability-magic-numbers)
                if( filter_by == filter_by::id || filter_by == filter_by::refs )
                    return std::stoul( value_str );
                return std::stoul( value_str, nullptr, 16 ); // NOLINT(readability-magic-numbers)
            }();
            filters.emplace_back( filter{ filter_by, comparison, value } );
        }
        return filters;
    }
    bool passes_filter( const cdc_lib::resource::ref_data::section_ref_data& a_section, filter a_filter )
    {
        switch( a_filter.by )
        {
            case filter_by::id:
                return perform_op( a_section.id, a_filter.value, a_filter.comparison );
            case filter_by::resource_type:
                return perform_op( a_section.resource_type, static_cast< std::uint8_t >( a_filter.value ), a_filter.comparison );
            case filter_by::section_type:
                return perform_op( a_section.section_type, 
                                   static_cast< cdc_lib::resource::cooked_resolve_section_type >( a_filter.value ),
                                   a_filter.comparison );
            case filter_by::refs:
                return perform_op( a_section.referenced_by_objects.size(),
                                   static_cast< std::size_t >( a_filter.value ),
                                   a_filter.comparison );
            case filter_by::size:
                return perform_op( a_section.size, static_cast< std::size_t >( a_filter.value ), a_filter.comparison );
            case filter_by::reloc_table_size:
                return perform_op( a_section.relocation_table_size, static_cast< std::size_t >( a_filter.value ), a_filter.comparison );
            case filter_by::none:
                throw std::runtime_error{"Invalid filter."};
        }
    }

    void sort( std::vector< cdc_lib::resource::ref_data::section_ref_data >& a_sections, filter_by a_sort_by )
    {
        if( a_sort_by == filter_by::none )
            return;
        std::sort( a_sections.begin(), a_sections.end(), [a_sort_by]( const auto& a_lhs, const auto& a_rhs )
        {
            switch( a_sort_by )
            {
                case filter_by::id:
                    return a_lhs.id < a_rhs.id;
                case filter_by::resource_type:
                    return a_lhs.resource_type < a_rhs.resource_type;
                case filter_by::section_type:
                    return a_lhs.section_type < a_rhs.section_type;
                case filter_by::refs:
                    return a_lhs.referenced_by_objects.size() < a_rhs.referenced_by_objects.size();
                case filter_by::size:
                    return a_lhs.size < a_rhs.size;
                case filter_by::reloc_table_size:
                    return a_lhs.relocation_table_size < a_rhs.relocation_table_size;
                case filter_by::none:
                    assert( false && "what the hell did you do" );
            }
        } );
    }

    int run( int a_argc, char** a_argv )
    {
        cxxopts::Options options( "resource_refdata", "Find" );
        options.add_options()
            ( "operation", "The operation to perform", cxxopts::value< std::string >() )
            ( "f,file", "The reference data file to use", cxxopts::value< std::string >() )
            ( "filter", "Filter the output", cxxopts::value< std::string >() )
            ( "s,sort", "Sort the output", cxxopts::value< std::string >()->default_value( "id" ) )
            ( "h,help", "Print help" );
        options.parse_positional( {"operation", "file", "filter"} );
        options.positional_help( "<file> <filter>" );
        auto result = options.parse( a_argc, a_argv );
        if( result.count( "help" ) != 0
         || result.count( "file" ) == 0
         || result.count( "filter" ) == 0 )
        {
            fmt::print( "{}\n", options.help() );
            return EXIT_FAILURE;
        }
        auto refdata = load_refdata( result["file"].as< std::string >() );
        auto filter = result["filter"].as< std::string >();
        auto filters = parse_filter( filter );
        auto sort_by = k_filter_by_lookup.lookup_or( result["sort"].as< std::string >(), filter_by::none );
        std::vector< cdc_lib::resource::ref_data::section_ref_data > sections_to_show{};
        for( const auto& section : refdata.sections )
        {
            bool filter_pass = true;
            for( const auto& filter : filters )
            {
                if( !passes_filter( section.second, filter ) )
                {
                    filter_pass = false;
                    break;
                }
            }
            if( filter_pass )
                sections_to_show.push_back( section.second );
        }
        sort( sections_to_show, sort_by );
        for( const auto& section : sections_to_show )
        {
            fmt::print( "id {:6}, type {:15}, rt {:2x}, size {:6x}, reloc {:4x}, refs {}\n",
                        section.id,
                        to_string( section.section_type ),
                        section.resource_type,
                        section.size,
                        section.relocation_table_size,
                        section.referenced_by_objects.size() );
        }
        return EXIT_SUCCESS;
    }
}

namespace operation_find_references
{
    int run( int a_argc, char** a_argv )
    {
        cxxopts::Options options( "resource_refdata", "Find references" );
        options.add_options()
            ( "operation", "The operation to perform", cxxopts::value< std::string >() )
            ( "f,file", "The reference data file to use", cxxopts::value< std::string >() )
            ( "s,section", "The section id to search for", cxxopts::value< int >() )
            ( "t,type", "The type id to search for", cxxopts::value< std::string >() )
            ( "h,help", "Print help" );
        options.parse_positional( {"operation", "file", "section", "type"} );
        options.positional_help( "<file> <section> <type>" );
        auto result = options.parse( a_argc, a_argv );
        if( result.count( "help" ) != 0 )
        {
            fmt::print( "{}\n", options.help() );
            return EXIT_FAILURE;
        }
        const auto section_type = string_to_section_type( result["type"].as< std::string >() );
        const auto section_id = static_cast< std::uint32_t >( result["section"].as< int >() );
        const auto section_guid = cdc_lib::resource::cooked_resource_guid{section_type, section_id};
        auto refdata = load_refdata( result["file"].as< std::string >() );
        const auto& section = refdata.sections.find( section_guid );
        if( section == refdata.sections.end() )
        {
            fmt::print( "could not find section in refdata.\n" );
            return EXIT_FAILURE;
        }
        fmt::print( "section is referenced by {} objects\n", section->second.referenced_by_objects.size() );
        for( const auto& object : section->second.referenced_by_objects )
        {
            const auto section_index =
                std::distance( object->references_sections.begin(),
                               std::ranges::find( object->references_sections, section_guid ) );
            assert( section_index != object->references_sections.size() );
            fmt::print( "referenced by object: {:64} as section {:5}\n", object->path, section_index );
        }
        return EXIT_SUCCESS;
    }
}

namespace operation_make_dtp_filelist
{
    constexpr std::uint32_t k_resource_type_soundplex = 0xd;
    
    int run( int a_argc, char** a_argv )
    {
        cxxopts::Options options( "resource_refdata", "Dump CSV" );
        options.add_options()
            ( "operation", "The operation to perform", cxxopts::value< std::string >() )
            ( "f,file", "The reference data file to use", cxxopts::value< std::string >() )
            ( "gamedir", "The game directory", cxxopts::value< std::string >() )
            ( "h,help", "Print help" );
        options.parse_positional( {"operation", "file", "gamedir"} );
        options.positional_help( "<file>" );
        auto result = options.parse( a_argc, a_argv );
        if( result.count( "help" ) != 0 )
        {
            fmt::print( "{}\n", options.help() );
            return EXIT_FAILURE;
        }
        auto refdata = load_refdata( result["file"].as< std::string >() );

        std::vector< cdc_lib::resource::ref_data::section_ref_data > soundplexes;
        for( const auto& section : refdata.sections )
        {
            if( section.second.resource_type == k_resource_type_soundplex )
                soundplexes.push_back( section.second );
        }
        std::ranges::sort( soundplexes, []( const auto& a_a, const auto& a_b ) { return a_a.id < a_b.id; } );

        const auto bigfile = result["gamedir"].as< std::string >() + "/bigfile.000.tiger";
        auto archive_stream = std::ifstream{bigfile, std::ios::binary};
        auto archive_ii = score::binary_io::create_input_interface( archive_stream );
        const auto archive = cdc_lib::file::tras::pc_w::load_archive( *archive_ii, bigfile );

        for( const auto& section : soundplexes )
        {
            const auto compressed_data = cdc_lib::file::tras::pc_w::read_offset( archive, section.offset, section.compressed_size );
            auto compressed_ii = score::binary_io::create_input_interface( compressed_data );
            const auto decompressed_data = cdc_lib::file::tras::pc_w::decompress_cdrm( *compressed_ii );
            const auto find_single = []( const std::string& a_string, const std::string& a_substring )
            {
                const auto first_pos = a_string.find( a_substring );
                const auto last_pos = a_string.rfind( a_substring );
                if( first_pos == last_pos )
                    return first_pos;
                return std::string::npos;
            };
            const auto find_one_of = [find_single]( const std::string& a_string, const std::vector< std::string >& a_substrings )
            {
                for( const auto& substring : a_substrings )
                {
                    const auto pos = find_single( a_string, substring );
                    if( pos != std::string::npos )
                        return pos;
                }
                return std::string::npos;
            };
            // order is important here!
            // mp NEEDS to be last because of cases like "vo\act_01\amelias_ca*mp*\"
            auto path_pos = find_one_of( decompressed_data, { "ambient\\",
                                                              "music\\",
                                                              "vo\\",
                                                              "character\\",
                                                              "event\\",
                                                              "event_triggered\\",
                                                              "object\\",
                                                              "ui\\",
                                                              "mp\\" } );
            if( path_pos == std::string::npos )
                fmt::print( "{},gr$\\audio\\snds\\<unknown>.snd\n", section.id );
            else
            {
                std::string filename = decompressed_data.substr( path_pos );
                auto null = filename.find( '\0' );
                if( null != std::string::npos )
                    filename = filename.substr( 0, null );
                fmt::print( "{},gr$\\audio\\snds\\{}.snd\n", section.id, filename );
            }
        }
        return EXIT_SUCCESS;
    }
}

namespace operation_dump_sections_csv
{
    int run( int a_argc, char** a_argv )
    {
        cxxopts::Options options( "resource_refdata", "Dump CSV" );
        options.add_options()
            ( "operation", "The operation to perform", cxxopts::value< std::string >() )
            ( "f,file", "The reference data file to use", cxxopts::value< std::string >() )
            ( "o,outfile", "The output CSV file to create", cxxopts::value< std::string >()->default_value( "dump.csv" ) )
            ( "h,help", "Print help" );
        options.parse_positional( {"operation", "file"} );
        options.positional_help( "<file>" );
        auto result = options.parse( a_argc, a_argv );
        if( result.count( "help" ) != 0 )
        {
            fmt::print( "{}\n", options.help() );
            return EXIT_FAILURE;
        }
        const auto filename = result["file"].as< std::string >();
        auto refdata = load_refdata( result["file"].as< std::string >() );
        std::ofstream out{result["outfile"].as< std::string >()};
        out.exceptions( std::ios::badbit | std::ios::eofbit | std::ios::failbit );
        fmt::print( out, "id,type,resource_type,referenced_by\n" );
        for( const auto& section : refdata.sections )
        {
            fmt::print( out,
                        "{},{},{},{}\n",
                        section.first.id,
                        to_string( section.first.type ),
                        section.second.resource_type,
                        section.second.referenced_by_objects.size() );
        }
        return EXIT_SUCCESS;
    }
}

void show_global_help()
{
    fmt::print( "Usage: resource_refdata <operation>\n" );
    fmt::print( "Operations:\n" );
    fmt::print( "  create [build]\n" );
    fmt::print( "  find-references [findref]\n" );
    fmt::print( "  find\n" );
    fmt::print( "  dump-sections-csv [section2csv]\n" );
    fmt::print( "  makedtplist\n" );
}

int main( int argc, char** argv )
{
    if( argc < 2 )
    {
        show_global_help();
        return EXIT_FAILURE;
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    std::string operation = argv[1];
    if( operation == "create" || operation == "build" )
        return operation_create::run( argc, argv );
    if( operation == "find" )
        return operation_find::run( argc, argv );
    if( operation == "find-references" || operation == "findref" )
        return operation_find_references::run( argc, argv );
    if( operation == "dump-sections-csv" || operation == "section2csv" )
        return operation_dump_sections_csv::run( argc, argv );
    if( operation == "makedtplist" )
        return operation_make_dtp_filelist::run( argc, argv );

    show_global_help();
    return EXIT_FAILURE;
}
