#ifndef CDC_LIB_SDEF_SDEF_PARSE_H
#define CDC_LIB_SDEF_SDEF_PARSE_H
#include <cdc_lib/sdef/sdef_node.h>
#include <string>

namespace cdc_lib::sdef
{
    sdef_node parse_sdef( const std::string& a_sdef_file_data );
}

#endif
