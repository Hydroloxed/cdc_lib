#ifndef CDC_LIB_RESOURCE_RSRC_RELOC_INPUT_STREAM_H
#define CDC_LIB_RESOURCE_RSRC_RELOC_INPUT_STREAM_H
#include "rsrc_relocation.h"
#include <cstdlib>
#include <score/binary_io/binary_io.h>
#include <span>
#include <stack>
#include <vector>

namespace cdc_lib::resource
{
    struct reloc_stream_subsection_info
    {
        std::string name{};
        std::size_t start_offset{0};
        std::size_t bytes_read{0};
    };

    class reloc_istream :
          public score::binary_io::input_interface
    {
        struct scope
        {
            scope() = default;
            /* NOTE:
             *      We shouldn't need to provide a constructor here,
             *      but sadly Clang doesn't support the proposal P0960 (part of C++20).
             *      This is needed for `emplace` container functions to work properly with aggregate initialization.
             *      As a workaround, we provide a user-defined constructor, which does the same task.
             */
            scope( std::size_t a_offset,
                   std::size_t a_bytes_read,
                   auto&& a_debug_name ) :
                offset{a_offset},
                bytes_read{a_bytes_read},
                debug_name{a_debug_name}
            {}
            std::size_t offset{0};
            std::size_t bytes_read{0};
            std::string debug_name{};
        };
    public:
        reloc_istream( input_interface& a_interface,
                       std::span< cooked_relocation > a_relocations,
                       std::size_t a_pointer_size ) :
            input_interface{a_interface.endian()},
            relocations{a_relocations},
            top_level_scope{ scope_stack.emplace( 0, 0, "<toplevel>" ) },
            underlying_interface{a_interface},
            pointer_size{a_pointer_size}
        {
            rebase();
        }
        reloc_istream( reloc_istream&& ) = default;
        reloc_istream( const reloc_istream& ) = delete;
        reloc_istream& operator=( const reloc_istream& ) = delete;
        reloc_istream& operator=( reloc_istream&& ) = delete;
        ~reloc_istream() override = default;
        void read( std::span< std::byte > a_out_bytes ) override;
        [[nodiscard]] std::size_t tell() const override;
        void seek( [[maybe_unused]] std::size_t a_offset ) override;

        void rebase();
        [[nodiscard]] std::span< reloc_stream_subsection_info > get_subsections() noexcept { return std::span{subsections}; }
        [[nodiscard]] std::span< const reloc_stream_subsection_info > get_subsections() const noexcept { return std::span{subsections}; }
        scope* start_scope( std::size_t a_offset, std::string_view a_debug_name = "<unnamed>" );
        scope* start_scope( std::string_view a_debug_name = "<unnamed>" );
        void end_scope();
        [[nodiscard]] cooked_relocation* get_relocation_at( std::size_t a_offset );
        [[nodiscard]] cooked_relocation* get_relocation_at_cursor();
        [[nodiscard]] cooked_relocation* try_read_relocation() noexcept;
        [[nodiscard]] cooked_relocation* read_relocation();
    private:
        void save_subsection( const scope& a_scope );
        [[nodiscard]] scope& get_current_scope();
        void update_scope();

        std::span< cooked_relocation > relocations;
        std::vector< reloc_stream_subsection_info > subsections{};
        std::stack< scope > scope_stack;
        scope& top_level_scope;
        input_interface& underlying_interface;
        const std::size_t pointer_size;

        friend class reloc_istream_scope;
    };

    class reloc_istream_scope
    {
    public:
        explicit reloc_istream_scope( reloc_istream& a_stream, std::string_view a_debug_name = "<unnamed>" ) :
            stream{a_stream},
            scope(stream.start_scope( a_debug_name ))
        {}
        reloc_istream_scope( reloc_istream& a_stream, std::size_t a_offset, std::string_view  a_debug_name = "<unnamed>" ) :
            stream{a_stream},
            scope(stream.start_scope( a_offset, a_debug_name ))
        {}
        reloc_istream_scope( reloc_istream_scope&& ) = default;
        reloc_istream_scope( const reloc_istream_scope& ) = delete;
        reloc_istream_scope& operator=( const reloc_istream_scope& ) = delete;
        reloc_istream_scope& operator=( reloc_istream_scope&& ) = delete;
        ~reloc_istream_scope()
        {
            if( scope )
                stream.end_scope();
        }
        explicit operator bool() const { return scope != nullptr; }
    private:
        reloc_istream& stream;
        reloc_istream::scope* scope;
    };
}

#endif
