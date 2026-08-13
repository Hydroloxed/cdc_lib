#ifndef CDC_LIB_SDEF_SDEF_TYPE_H
#define CDC_LIB_SDEF_SDEF_TYPE_H
#include <string>

namespace cdc_lib::sdef
{
    enum class sdef_primitive_type
    {
        none,
        bool8,
        int8,
        uint8,
        int16,
        uint16,
        int32,
        uint32,
        int64,
        uint64,
        float32,
        float64
    };

    struct sdef_type_ref
    {
        std::string type_name{};
        sdef_primitive_type primitive_type{sdef_primitive_type::none};
    };
}

#endif
