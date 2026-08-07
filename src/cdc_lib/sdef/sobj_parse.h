#ifndef CDC_LIB_SDEF_SOBJ_PARSE_H
#define CDC_LIB_SDEF_SOBJ_PARSE_H
#include <cdc_lib/resource/rsrc_reloc_input_stream.h>
#include <cdc_lib/sdef/sobj_node.h>

namespace cdc_lib::sdef
{
    sobj_node parse_sobj( sdef_node* a_root_struct,
                          resource::reloc_istream& a_data );
}

#endif
