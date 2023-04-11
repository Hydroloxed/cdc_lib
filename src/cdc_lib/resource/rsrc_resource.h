#ifndef CDC_LIB_RESOURCE_RSRC_RESOURCE_H
#define CDC_LIB_RESOURCE_RSRC_RESOURCE_H
#include <concepts>
#include <cstdint>
#include <functional>

namespace cdc_lib::resource
{
    class resource;
    using resource_id = std::uint64_t;
    using resource_lookup = std::function< resource*( resource_id ) >;

    class resource_ref
    {
    public:
        resource_ref() = default;
        explicit resource_ref( std::uint64_t a_user_id ) :
            user_id( a_user_id )
        {}
        [[nodiscard]] bool is_concrete_reference() const { return resource_ != nullptr; }
        [[nodiscard]] bool is_null_reference() const { return user_id == 0; }
        [[nodiscard]] resource* get_resource() { return resource_; }
        [[nodiscard]] const resource* get_resource() const { return resource_; }
        [[nodiscard]] std::uint64_t get_user_id() const { return user_id; }
        resource* lookup( std::invocable auto a_lookup )
        {
            resource_ = a_lookup( user_id );
            return resource_;
        }
    private:
        resource_id user_id{0};
        resource*   resource_{nullptr};
    };
}

#endif
