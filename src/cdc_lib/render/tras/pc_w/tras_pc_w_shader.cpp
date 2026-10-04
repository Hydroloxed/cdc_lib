#include "tras_pc_w_shader.h"
#include <algorithm>
#include <map>

namespace cdc_lib::render::tras::pc_w
{
    std::unique_ptr< shader_table > load_shader_table( score::binary_io::input_interface& a_input )
    {
        auto table = std::make_unique< shader_table >();
        table->type = static_cast< shader_type >( read< std::uint32_t >( a_input ) );

        std::uint32_t base_offset = a_input.tell();

        const auto offset_table_size = read< std::uint32_t >( a_input );
        [[maybe_unused]] const auto total_data_size = read< std::uint32_t >( a_input );

        table->shaders.reserve( offset_table_size >> 2u );
        std::vector< std::uint32_t > shader_offsets;
        for( std::uint32_t i = 0; i < offset_table_size; i += 4 )
        {
            shader_offsets.push_back( read< std::uint32_t >( a_input ) );
        }

        std::vector< std::uint32_t > unique_shader_offsets( shader_offsets );
        std::ranges::sort( unique_shader_offsets );
        unique_shader_offsets.erase( std::ranges::unique( unique_shader_offsets ).begin(), unique_shader_offsets.end() );
        std::map< std::uint32_t, std::shared_ptr< shader > > offset_to_shader;
        for( const auto offset : unique_shader_offsets )
        {
            if( offset == static_cast< std::uint32_t >( -1 ) )
                continue;
            a_input.seek( offset + base_offset );
            const auto cur = std::make_shared< shader >();
            cur->id.hi = read< std::uint32_t >( a_input );
            cur->id.size = read< std::uint32_t >( a_input );
            cur->id.lo = read< std::uint64_t >( a_input );
            cur->data.resize( cur->id.size );
            a_input.read( std::span{ cur->data.data(), cur->id.size - sizeof( shader_id ) } );
            offset_to_shader[offset] = cur;
        }
        for( const auto offset : shader_offsets )
        {
            if( offset != static_cast< std::uint32_t >( -1 ) )
                table->shaders.push_back( offset_to_shader.at( offset ) );
            else
                table->shaders.push_back( std::shared_ptr< shader >{} );
        }
        return table;
    }
}
