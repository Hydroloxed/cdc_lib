#ifndef CDC_LIB_RESOURCE_RSRC_RESOURCE_DB_H
#define CDC_LIB_RESOURCE_RSRC_RESOURCE_DB_H
#include <any>
#include <cdc_lib/file/archive_fs.h>
#include <cdc_lib/resource/rsrc_resolve_object.h>
#include <map>
#include <optional>
#include <score/binary_io/binary_io.h>
#include <unordered_map>
#include <utility>

namespace cdc_lib::resource
{
    class resource_db;
    class db_object;

    class resource
    {
    public:
        explicit resource( const cooked_resolve_section& a_section_metadata ) : section_metadata{a_section_metadata} {}
        resource() = default;
        resource( const resource& ) = default;
        resource& operator=( const resource& ) = default;
        resource( resource&& ) = default;
        resource& operator=( resource&& ) = default;
        ~resource() = default;

        [[nodiscard]] cooked_resource_guid guid() const noexcept { return {section_metadata.type, section_metadata.id}; }
        [[nodiscard]] bool is_loaded() const noexcept { return loaded; }
        [[nodiscard]] std::optional< std::string > get_raw_data() const noexcept { return raw_data; }
        [[nodiscard]] std::span< const db_object* const > get_referenced_by_objects() const noexcept; // must be defined after db_object
        [[nodiscard]] const cooked_resolve_section& get_section_metadata() const noexcept { return section_metadata; }

        void load_sync();


    private:
        cooked_resolve_section section_metadata{};
        resource_db* db{};
        bool loaded{false};
        std::any runtime_data{};
        std::optional< std::string > raw_data{};
        std::vector< db_object* > referenced_by_objects{};

        friend class resource_db;
    };

    class db_object
    {
    public:
        db_object() = delete;
        explicit db_object( std::string_view a_path ) : path{a_path} {}
        db_object( const db_object& ) = default;
        db_object& operator=( const db_object& ) = default;
        db_object( db_object&& ) = default;
        db_object& operator=( db_object&& ) = default;
        ~db_object() = default;
        [[nodiscard]] bool is_loaded() const;
        [[nodiscard]] std::string_view get_path() const { return path; }
        [[nodiscard]] resource* get_primary_resource();
        [[nodiscard]] cooked_resource_guid get_primary_resource_ref() const { return primary_section; }
        void set_primary_resource_ref( cooked_resource_guid a_guid ) { primary_section = a_guid; }
        [[nodiscard]] std::optional< std::size_t > find_section_index( cooked_resource_guid a_guid ) const;
        [[nodiscard]] std::span< const cooked_resource_guid > iterate_resource_references() const noexcept { return std::span{referenced_resources}; }
        void add_resource_ref( cooked_resource_guid a_guid ) { referenced_resources.push_back( a_guid ); }
        void load_sync();

    private:
        resource_db* db{};
        std::string path{};
        std::vector< cooked_resource_guid > referenced_resources{};
        cooked_resource_guid primary_section{};

        friend class resource_db;
    };

    class resource_db
    {
    public:
        resource_db() = default;
        resource_db( const resource_db& ) = delete;
        resource_db& operator=( const resource_db& ) = delete;
        resource_db( resource_db&& ) = default;
        resource_db& operator=( resource_db&& ) = default;
        ~resource_db() = default;

        [[nodiscard]] bool empty() const { return resources.empty(); }
        [[nodiscard]] bool contains( cooked_resource_guid a_guid ) const { return resources.contains( a_guid ); }
        void insert( cooked_resource_guid a_guid, resource&& a_resource );
        void insert( std::string_view a_path, db_object&& a_object );
        void insert( db_object&& a_object ) { insert( a_object.get_path(), std::move( a_object ) ); }
        [[nodiscard]] resource& at( cooked_resource_guid a_guid ) { return resources.at( a_guid ); }
        [[nodiscard]] const resource& at( cooked_resource_guid a_guid ) const { return resources.at( a_guid ); }
        [[nodiscard]] std::size_t size() const { return resources.size(); }
        [[nodiscard]] std::size_t objects_size() const { return db_objects.size(); }
        [[nodiscard]] auto begin() { return resources.begin(); }
        [[nodiscard]] auto end() { return resources.end(); }
        [[nodiscard]] auto begin() const { return resources.begin(); }
        [[nodiscard]] auto end() const { return resources.end(); }

        [[nodiscard]] const std::unordered_map< cooked_resource_guid, resource >& iterate_resources() const { return resources; }
        [[nodiscard]] std::unordered_map< cooked_resource_guid, resource >& iterate_resources() { return resources; }
        [[nodiscard]] const std::map< std::string, db_object >& iterate_objects() const { return db_objects; }

        [[nodiscard]] std::string load_offset_sync( std::uint64_t a_offset, std::size_t a_size );
        void init_archive_from_gamedir( std::string_view a_gamedir );

        void add_resource_references();

    private:
        std::unordered_map< cooked_resource_guid, resource > resources{};
        std::map< std::string, db_object > db_objects{};
        file::archive_multifs archive_fs{};
    };

    void save_db( score::binary_io::output_interface& a_output, const resource_db& a_db );
    resource_db load_db( score::binary_io::input_interface& a_input );
}

#endif
