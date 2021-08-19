#include "wit/wit.hpp"
#include <cassert>
#include <memory>

template<typename Alloc = std::allocator<char>>
struct alloc_aware_impl
{
    using allocator_type = std::allocator<char>;
    using string_t = std::basic_string<char, std::char_traits<char>, allocator_type>;
    string_t str;

    alloc_aware_impl(std::allocator_arg_t, Alloc const& a, const char* str = "") : str(str, a)
    {
    }
};

using alloc_aware = wit::wit<alloc_aware_impl<>>;

int main()
{
    alloc_aware x;
    assert(x.str == "");

    alloc_aware y = "test";
    assert(y.str == "test");

    alloc_aware z("test2", std::allocator<char>());
    assert(z.str == "test2");

    assert(true);
}
