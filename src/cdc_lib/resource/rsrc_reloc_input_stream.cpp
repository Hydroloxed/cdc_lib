#include "rsrc_reloc_input_stream.h"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <span>
#include <stdexcept>
#include <string_view>

namespace cdc_lib::resource
{
    void reloc_istream::read( std::span< std::byte > a_out_bytes )
    {
        assert( underlying_interface.tell() == get_current_scope().offset + get_current_scope().bytes_read );
        underlying_interface.read( a_out_bytes );
        get_current_scope().bytes_read += a_out_bytes.size();
    }

    [[nodiscard]] std::size_t reloc_istream::tell() const
    {
        return underlying_interface.tell() - top_level_scope.offset;
    }

    void reloc_istream::seek( [[maybe_unused]] std::size_t a_offset )
    {
        assert( 0 && "You can't use seek() with a reloc_input_stream."
                     "If you want to skip n bytes, use skip_bytes instead (from binary_io)" );
    }

    void reloc_istream::rebase()
    {
        top_level_scope.offset = underlying_interface.tell();
    }

    reloc_istream::scope* reloc_istream::start_scope( std::size_t a_offset, std::string_view a_debug_name )
    {
        auto* s = &scope_stack.emplace( a_offset, 0, std::string{ a_debug_name } );
        update_scope();
        return s;
    }

    reloc_istream::scope* reloc_istream::start_scope( std::string_view a_debug_name )
    {
        const auto* relocation_here = try_read_relocation();
        if( !relocation_here || relocation_here->is_external() )
            return nullptr;
        return start_scope( relocation_here->dest_ptr_offset + top_level_scope.offset, a_debug_name );
    }

    void reloc_istream::end_scope()
    {
        assert( !scope_stack.empty() && "Relocation scope stack underflow!" );
        save_subsection( scope_stack.top() );
        scope_stack.pop();
        update_scope();
    }

    [[nodiscard]] cooked_relocation* reloc_istream::get_relocation_at( std::size_t a_offset )
    {
        const auto find = std::ranges::find_if
        (
            relocations,
            [a_offset]( const cooked_relocation& a_reloc )
            {
                return a_reloc.src_ptr_offset == a_offset;
            }
        );
        if( find == std::end( relocations ) )
            return nullptr;
        return &*find;
    }

    [[nodiscard]] cooked_relocation* reloc_istream::get_relocation_at_cursor()
    {
        return get_relocation_at( underlying_interface.tell() - top_level_scope.offset );
    }

    [[nodiscard]] cooked_relocation* reloc_istream::try_read_relocation() noexcept
    {
        auto* reloc = get_relocation_at_cursor();
        get_current_scope().bytes_read += pointer_size;
        underlying_interface.seek( underlying_interface.tell() + pointer_size );
        return reloc;
    }

    [[nodiscard]] cooked_relocation* reloc_istream::read_relocation()
    {
        auto* reloc = try_read_relocation();
        if( !reloc )
            throw std::range_error{ "Could not read relocation." };
        return reloc;
    }


    void reloc_istream::save_subsection( const scope& a_scope )
    {
        subsections.push_back( {a_scope.debug_name, a_scope.offset, a_scope.bytes_read} );
    }
    [[nodiscard]] reloc_istream::scope& reloc_istream::get_current_scope()
    {
        return scope_stack.empty() ? top_level_scope : scope_stack.top();
    }
    void reloc_istream::update_scope()
    {
        underlying_interface.seek( scope_stack.top().offset + scope_stack.top().bytes_read );
    }
}