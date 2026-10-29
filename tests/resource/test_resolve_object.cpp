#include <boost/ut.hpp>
#include <cdc_lib/resource/rsrc_reloc_input_stream.h>
#include <cdc_lib/resource/tras/pc_w/tras_pc_w_resolve_object.h>
#include <score/binary_io/binary_io.h>

using namespace boost::ut;

namespace
{
    auto load_ro(const char* a_data, std::size_t a_size)
    {
        std::string_view data_str{a_data, a_size};
        auto ii = score::binary_io::create_input_interface(data_str);
        return cdc_lib::resource::tras::pc_w::load_object( *ii );
    }
}

suite< "resource_resolve_object" > suite_resolve_object = []
{
    test("empty") = []
    {
        const char data[] = "\x16\0\0\0" // versionNumber
                            "\0\0\0\0" // drmIncludeLength
                            "\0\0\0\0" // drmDepLength
                            "\0\0\0\0" // paddingLength
                            "\0\0\0\0" // projectedDRMSize
                            "\0\0\0\0" // flags
                            "\0\0\0\0" // numSections
                            "\xff\xff\xff\xff" // primarySection
                             ;
        auto object = load_ro(data, sizeof data);
        expect(eq(object->sections.size(), 0));
        expect(eq(object->includes.size(), 0));
        expect(eq(object->objects_that_depend_on.size(), 0));
    };

    test("-1 means no primary sections") = []
    {
        const char data[] = "\x16\0\0\0" // versionNumber
                            "\0\0\0\0" // drmIncludeLength
                            "\0\0\0\0" // drmDepLength
                            "\0\0\0\0" // paddingLength
                            "\0\0\0\0" // projectedDRMSize
                            "\0\0\0\0" // flags
                            "\0\0\0\0" // numSections
                            "\xff\xff\xff\xff" // primarySection
                             ;
        auto object = load_ro(data, sizeof data);
        expect(eq(object->primary_section, nullptr));
    };

    // FIXME: Should throw, not assert
    skip / test("throws on wrong version number") = []
    {
        const char data[] = "\x15\0\0\0" // versionNumber
                            "\0\0\0\0" // drmIncludeLength
                            "\0\0\0\0" // drmDepLength
                            "\0\0\0\0" // paddingLength
                            "\0\0\0\0" // projectedDRMSize
                            "\0\0\0\0" // flags
                            "\0\0\0\0" // numSections
                            "\xff\xff\xff\xff" // primarySection
                             ;
        expect(throws([&data]{load_ro(data, sizeof data); }));
    };

    test("single section") = []
    {
        const char data[] = "\x16\0\0\0" // versionNumber
                            "\0\0\0\0" // drmIncludeLength
                            "\0\0\0\0" // drmDepLength
                            "\0\0\0\0" // paddingLength
                            "\0\0\0\0" // projectedDRMSize
                            "\0\0\0\0" // flags
                            "\1\0\0\0" // numSections
                            "\xff\xff\xff\xff" // primarySection
                            // begin SectionInfo 1
                            "\x12\x34\00\00" // size
                            "\0" // type - TODO
                            "\0" // misc flags
                            "\0\0" // versionID
                            "\0\0\0\0" // packedData
                            "\x56\0\0\0" // id
                            "\xff\xff\xff\xff" // specMask
                            // begin SectionExtraInfo 1
                            "\x56\0\0\0" // uniqueId
                            "\x11\x11\x11\x11" // packedOffset
                            "\x22\x22\x22\x22" // compressedSize
                            "\x33\x33\x33\x33" // decompressedOffset
                             ;
        auto object = load_ro(data, sizeof data);
        expect(eq(object->sections.size(), 1));
        expect(eq(object->sections.at(0).size, 0x3412));
        expect(eq(object->sections.at(0).id, 0x56));
        expect(eq(object->sections.at(0).extra_data.packed_offset, 0x11111111));
        expect(eq(object->sections.at(0).extra_data.compressed_size, 0x22222222));
        expect(eq(object->sections.at(0).extra_data.decompressed_offset, 0x33333333));
    };
};
