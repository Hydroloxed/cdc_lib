#ifndef CDC_LIB_RESOURCE_RSRC_RELOC_INPUT_STREAM_H
#define CDC_LIB_RESOURCE_RSRC_RELOC_INPUT_STREAM_H
#include "rsrc_relocation.h"
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <exception>
#include <optional>
#include <score/binary_io/binary_io.h>
#include <stack>

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
        void read( std::span< std::byte > a_out_bytes ) override
        {
            assert( underlying_interface.tell() == get_current_scope().offset + get_current_scope().bytes_read );
            underlying_interface.read( a_out_bytes );
            get_current_scope().bytes_read += a_out_bytes.size();
        }
        [[nodiscard]] std::size_t tell() const override { return underlying_interface.tell() - top_level_scope.offset; }
        void seek( [[maybe_unused]] std::size_t a_offset ) override
        {
            assert( 0 && "You can't use seek() with a reloc_input_stream."
                         "If you want to skip n bytes, use skip_bytes instead (from binary_io)" );
        }

        void rebase() { top_level_scope.offset = underlying_interface.tell(); }
        [[nodiscard]] std::span< reloc_stream_subsection_info > get_subsections() noexcept { return std::span{subsections}; }
        [[nodiscard]] std::span< const reloc_stream_subsection_info > get_subsections() const noexcept { return std::span{subsections}; }
        scope* start_scope( std::size_t a_offset, std::string_view a_debug_name = "<unnamed>" )
        {
            auto* s = &scope_stack.emplace( a_offset, 0, std::string{ a_debug_name } );
            update_scope();
            return s;
        }
        scope* start_scope( std::string_view a_debug_name = "<unnamed>" )
        {
            auto* relocation_here = try_read_relocation();
            if( !relocation_here || relocation_here->is_external() )
                return nullptr;
            return start_scope( relocation_here->dest_ptr_offset + top_level_scope.offset, a_debug_name );
        }
        void end_scope()
        {
            assert( !scope_stack.empty() && "Relocation scope stack underflow!" );
            save_subsection( scope_stack.top() );
            scope_stack.pop();
            update_scope();
        }
        [[nodiscard]] cooked_relocation* get_relocation_at( std::size_t a_offset )
        {
            const auto find = std::find_if
            (
                relocations.begin(),
                relocations.end(),
                [a_offset]( const cooked_relocation& a_reloc )
                {
                    return a_reloc.src_ptr_offset == a_offset;
                }
            );
            if( find == std::end( relocations ) )
                return nullptr;
            return &*find;
        }
        [[nodiscard]] cooked_relocation* get_relocation_at_cursor()
        {
            return get_relocation_at( underlying_interface.tell() - top_level_scope.offset );
        }
        [[nodiscard]] cooked_relocation* try_read_relocation() noexcept
        {
            auto* reloc = get_relocation_at_cursor();
            get_current_scope().bytes_read += pointer_size;
            underlying_interface.seek( underlying_interface.tell() + pointer_size );
            return reloc;
        }
        [[nodiscard]] cooked_relocation* read_relocation()
        {
            auto* reloc = try_read_relocation();
            if( !reloc )
                throw std::range_error{ "Could not read relocation." };
            return reloc;
        }
    private:
        void save_subsection( const scope& a_scope )
        {
            subsections.push_back( {a_scope.debug_name, a_scope.offset, a_scope.bytes_read} );
        }
        [[nodiscard]] scope& get_current_scope()
        {
            return scope_stack.empty() ? top_level_scope : scope_stack.top();
        }
        void update_scope()
        {
            underlying_interface.seek( scope_stack.top().offset + scope_stack.top().bytes_read );
        }

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
