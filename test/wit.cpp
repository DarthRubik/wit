#include "wit/wit.hpp"
#include <cassert>
#include <memory>

template<typename T, typename POCC, typename POCCA, typename POCMA, typename POCS>
struct debug_allocator
{
    std::string id;
    using value_type = T;

    debug_allocator() : id("default") {}
    debug_allocator(std::string id) : id(std::move(id)) {}

    template <class U>
    debug_allocator(debug_allocator<U, POCC, POCCA, POCMA, POCS> const& other)
        : id(other.id) {}

    // Allocators move constructor must not actually move (call the copy
    // construct)
    template <class U>
    debug_allocator(debug_allocator<U, POCC, POCCA, POCMA, POCS>&& other)
        : debug_allocator(other) {}

    T* allocate(std::size_t n)
    {
        return static_cast<T*>(operator new (n*sizeof(value_type)));
    }

    void deallocate(T* p, std::size_t) noexcept
    {
        operator delete(p);
    }

    debug_allocator select_on_container_copy_construction() const
    {
        if (POCC::value)
        {
            return *this;
        }
        else
        {
            return debug_allocator();
        }
    }

    using propagate_on_container_copy_assignment = POCCA;
    using propagate_on_container_move_assignment = POCMA;
    using propagate_on_container_swap            = POCS;
};

template<typename T, typename U, typename POCC, typename POCCA, typename POCMA, typename POCS>
bool operator==(debug_allocator<T, POCC, POCCA, POCMA, POCS> const& x,
                debug_allocator<U, POCC, POCCA, POCMA, POCS> const& y) noexcept
{
    return x.id == y.id;
}

template<typename T, typename U, typename POCC, typename POCCA, typename POCMA, typename POCS>
bool operator!=(debug_allocator<T, POCC, POCCA, POCMA, POCS> const& x,
                debug_allocator<U, POCC, POCCA, POCMA, POCS> const& y) noexcept
{
    return !(x == y);
}

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


template<typename alloc_aware>
void smoke_test()
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
template<template<class> class alloc_aware>
void test_copy_construct()
{
    {
        using a = debug_allocator<
            char, std::false_type, std::false_type, std::false_type, std::false_type>;

        alloc_aware<a> x = a("a1");
        assert(x.get_allocator().id == "a1");

        alloc_aware<a> y = x;
        assert(y.get_allocator().id == "default");

        alloc_aware<a> z = alloc_aware<a>(x, a("a2"));
        assert(z.get_allocator().id == "a2");
    }
    {
        using a = debug_allocator<
            char, std::true_type, std::false_type, std::false_type, std::false_type>;

        alloc_aware<a> x = a("a1");
        alloc_aware<a> y = x;
        assert(y.get_allocator().id == "a1");
    }
}

template<template<class> class alloc_aware>
void test_move_construct()
{
    using a = debug_allocator<
        char, std::false_type, std::false_type, std::false_type, std::false_type>;

    alloc_aware<a> x = a("a1");
    assert(x.get_allocator().id == "a1");

    alloc_aware<a> y = std::move(x);
    assert(y.get_allocator().id == "a1");

    x = a("a1");
    alloc_aware<a> z = alloc_aware<a>(std::move(x), a("a2"));
    assert(z.get_allocator().id == "a2");
}

template<typename Alloc>
using alloc_aware = wit::wit<alloc_aware_impl<Alloc>>;

int main()
{
    smoke_test<alloc_aware<std::allocator<char>>>();
    test_copy_construct<alloc_aware>();
    test_move_construct<alloc_aware>();
}


