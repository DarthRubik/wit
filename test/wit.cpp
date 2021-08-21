#include "wit/wit.hpp"
#include "cma/debug_allocator.hpp"
#include <cassert>
#include <memory>
#include <cstring>
#include <map>


void fail()
{
    assert(false);
}

template<typename POCC,
         typename POCCA,
         typename POCMA,
         typename POCS>
using debug_allocator = cma::debug_allocator<char, POCC, POCCA, POCMA, POCS, fail>;


template<typename Alloc = std::allocator<char>>
struct alloc_aware_impl
{
private:
    Alloc alloc;

    using traits = std::allocator_traits<Alloc>;

    static_assert(std::is_same<typename traits::value_type, char>::value, "");

public:

    char* str;

    // These are deleted for the test...They don't need to be in reality
    alloc_aware_impl(alloc_aware_impl const&) = delete;
    alloc_aware_impl(alloc_aware_impl&&) = delete;
    alloc_aware_impl& operator=(alloc_aware_impl const &) = delete;
    alloc_aware_impl& operator=(alloc_aware_impl&&) = delete;


    // The methods and member bellow are the minimum required in order to use
    // the "wit" adapter

    using allocator_type = Alloc;
    allocator_type get_allocator() const
    {
        return alloc;
    }

    alloc_aware_impl(Alloc const& a, const char* str = "") : alloc(a)
    {
        this->str = traits::allocate(alloc, std::strlen(str) + 1);
        std::strcpy(this->str, str);
    }
    alloc_aware_impl(Alloc const& a, alloc_aware_impl const& other)
        : alloc_aware_impl(a, other.str) {}

    // Implements a "real" swap, that always propogates allocators.....the wit
    // adapter will the decide what the allocator wants to do and respond
    // appropriately
    friend void really_swap(alloc_aware_impl& a, alloc_aware_impl& b)
    {
        using std::swap;

        swap(a.str, b.str);
        swap(a.alloc, b.alloc);
    }

    ~alloc_aware_impl()
    {
        traits::deallocate(alloc, this->str, std::strlen(this->str)+1);
    }
};


template<typename alloc_aware>
void smoke_test()
{
    auto alloc = std::allocator<char>();

    alloc_aware x;
    assert(std::string(x.str) == "");

    x = alloc_aware(std::move(alloc));
    assert(std::string(x.str) == "");
    x = alloc_aware(alloc);
    assert(std::string(x.str) == "");

    x = "test";
    assert(std::string(x.str) == "test");

    x = alloc_aware("test2", std::move(alloc));
    assert(std::string(x.str) == "test2");
    x = alloc_aware("test2", alloc);
    assert(std::string(x.str) == "test2");

    x = alloc_aware(std::allocator_arg, std::move(alloc), "test3");
    assert(std::string(x.str) == "test3");
    x = alloc_aware(std::allocator_arg, alloc, "test3");
    assert(std::string(x.str) == "test3");

    alloc_aware y = x;
    assert(std::string(y.str) == "test3");

    y = alloc_aware(x, std::move(alloc));
    assert(std::string(y.str) == "test3");
    y = alloc_aware(x, alloc);
    assert(std::string(y.str) == "test3");

    y = std::move(x);
    assert(std::string(y.str) == "test3");

    x = alloc_aware("test3");
    y = alloc_aware(std::move(x), std::move(alloc));
    assert(std::string(y.str) == "test3");
    x = alloc_aware("test3");
    y = alloc_aware(std::move(x), alloc);
    assert(std::string(y.str) == "test3");

}
template<template<class> class alloc_aware>
void test_copy_construct()
{
    {
        using a = debug_allocator<
            std::false_type, std::false_type, std::false_type, std::false_type>;

        alloc_aware<a> x("x", a("a1"));
        assert(x.get_allocator().id() == "a1");

        alloc_aware<a> y = x;
        assert(y.get_allocator().id() == "default");
        assert(std::string(y.str) == "x");

        alloc_aware<a> z = alloc_aware<a>(x, a("a2"));
        assert(z.get_allocator().id() == "a2");
        assert(std::string(z.str) == "x");
    }
    {
        using a = debug_allocator<
            std::true_type, std::false_type, std::false_type, std::false_type>;

        alloc_aware<a> x = a("a1");
        alloc_aware<a> y = x;
        assert(y.get_allocator().id() == "a1");
    }
}

template<template<class> class alloc_aware>
void test_move_construct()
{
    using a = debug_allocator<
        std::false_type, std::false_type, std::false_type, std::false_type>;

    alloc_aware<a> x("x", a("a1"));
    assert(x.get_allocator().id() == "a1");

    alloc_aware<a> y = std::move(x);
    assert(std::string(y.str) == "x");
    assert(y.get_allocator().id() == "a1");

    x = alloc_aware<a>("x", a("a1"));
    alloc_aware<a> z = alloc_aware<a>(std::move(x), a("a2"));
    assert(std::string(z.str) == "x");
    assert(z.get_allocator().id() == "a2");
}

template<template<class> class alloc_aware>
void test_copy_assign()
{
    {
        using a = debug_allocator<
            std::false_type, std::false_type, std::false_type, std::false_type>;

        alloc_aware<a> x = a("a1");
        alloc_aware<a> y("y", a("a2"));

        x = y;
        assert(std::string(x.str) == "y");
        assert(x.get_allocator().id() == "a1");
    }
    {
        using a = debug_allocator<
            std::false_type, std::true_type, std::false_type, std::false_type>;

        alloc_aware<a> x = a("a1");
        alloc_aware<a> y("y", a("a2"));

        x = y;
        assert(std::string(x.str) == "y");
        assert(x.get_allocator().id() == "a2");
    }
}

template<template<class> class alloc_aware>
void test_move_assign()
{
    {
        using a = debug_allocator<
            std::false_type, std::false_type, std::false_type, std::false_type>;

        alloc_aware<a> x = a("a1");
        alloc_aware<a> y("y", a("a2"));

        x = std::move(y);
        assert(std::string(x.str) == "y");
        assert(x.get_allocator().id() == "a1");
    }
    {
        using a = debug_allocator<
            std::false_type, std::false_type, std::true_type, std::false_type>;

        alloc_aware<a> x = a("a1");
        alloc_aware<a> y("y", a("a2"));

        x = std::move(y);
        assert(std::string(x.str) == "y");
        assert(x.get_allocator().id() == "a2");
    }
    {
        using a = debug_allocator<
            std::false_type, std::false_type, std::false_type, std::false_type>;

        a alloc = a("a1");
        alloc_aware<a> x = alloc;
        alloc_aware<a> y("y", alloc);

        x = std::move(y);
        assert(std::string(x.str) == "y");
        assert(x.get_allocator().id() == "a1");
    }
}

template<template<class> class alloc_aware>
void test_swap()
{
    {
        using a = debug_allocator<
            std::false_type, std::false_type, std::false_type, std::false_type>;

        alloc_aware<a> x("x", a("a1"));
        alloc_aware<a> y("y", a("a2"));

        swap(x, y);

        assert(std::string(x.str) == "y");
        assert(std::string(y.str) == "x");
        assert(x.get_allocator().id() == "a1");
        assert(y.get_allocator().id() == "a2");
    }
    {
        using a = debug_allocator<
            std::false_type, std::false_type, std::false_type, std::true_type>;

        alloc_aware<a> x("x", a("a1"));
        alloc_aware<a> y("y", a("a2"));

        swap(x, y);

        assert(std::string(x.str) == "y");
        assert(std::string(y.str) == "x");
        assert(x.get_allocator().id() == "a2");
        assert(y.get_allocator().id() == "a1");
    }
    {
        using a = debug_allocator<
            std::false_type, std::false_type, std::false_type, std::false_type>;

        a alloc = a("a1");
        alloc_aware<a> x("x", alloc);
        alloc_aware<a> y("y", alloc);

        swap(x, y);

        assert(std::string(x.str) == "y");
        assert(std::string(y.str) == "x");
        assert(x.get_allocator().id() == "a1");
        assert(y.get_allocator().id() == "a1");
    }
}

template<typename Alloc>
using alloc_aware = wit::wit<alloc_aware_impl<Alloc>>;

int main()
{
    smoke_test<alloc_aware<std::allocator<char>>>();
    test_copy_construct<alloc_aware>();
    test_move_construct<alloc_aware>();
    test_copy_assign<alloc_aware>();
    test_move_assign<alloc_aware>();
    test_swap<alloc_aware>();
}


