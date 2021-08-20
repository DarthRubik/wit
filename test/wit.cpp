#include "wit/wit.hpp"
#include <cassert>
#include <memory>

template<typename Alloc = std::allocator<char>>
struct alloc_aware_impl
{
    using allocator_type = Alloc;

    allocator_type get_allocator() const
    {
        return str.get_allocator();
    }
    using string_t = std::basic_string<char, std::char_traits<char>, allocator_type>;
    string_t str;

    alloc_aware_impl(std::allocator_arg_t, Alloc const& a, const char* str = "") : str(str, a)
    {
    }
    alloc_aware_impl(alloc_aware_impl const& other, Alloc const& a)
        : str(other.str, a) {}
};

using alloc_aware = wit::wit<alloc_aware_impl<>>;

int main()
{
    auto alloc = std::allocator<char>();

    alloc_aware x;
    assert(x.str == "");

    x = alloc_aware(std::move(alloc));
    assert(x.str == "");
    x = alloc_aware(alloc);
    assert(x.str == "");

    x = "test";
    assert(x.str == "test");

    x = alloc_aware("test2", std::move(alloc));
    assert(x.str == "test2");
    x = alloc_aware("test2", alloc);
    assert(x.str == "test2");

    x = alloc_aware(std::allocator_arg, std::move(alloc), "test3");
    assert(x.str == "test3");
    x = alloc_aware(std::allocator_arg, alloc, "test3");
    assert(x.str == "test3");

    alloc_aware y = x;
    assert(y.str == "test3");

    y = alloc_aware(x, std::move(alloc));
    assert(y.str == "test3");
    y = alloc_aware(x, alloc);
    assert(y.str == "test3");

    y = std::move(x);
    assert(y.str == "test3");

    x.str = "test3";
    y = alloc_aware(std::move(x), std::move(alloc));
    assert(y.str == "test3");
    x.str = "test3";
    y = alloc_aware(std::move(x), alloc);
    assert(y.str == "test3");
}


