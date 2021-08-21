#include "wit/wit.hpp"
#include <cassert>
#include <memory>
#include <cstring>
#include <map>

struct debug_allocator_info
{
    // For debug purposes
    std::string id;

    // Maps allocations to sizes of the array
    std::map<void*, std::size_t> allocations;

    ~debug_allocator_info()
    {
        // When we are deleted, we should have no allocations left
        assert(allocations.empty());
    }
};

template<typename T, typename POCC, typename POCCA, typename POCMA, typename POCS>
struct debug_allocator
{
    // So that copying it extends the life of the "info"
    std::shared_ptr<debug_allocator_info> info;
    using value_type = T;

    std::string id() const
    {
        return info->id;
    }

    debug_allocator() : debug_allocator("default") {}
    debug_allocator(std::string id)
        : info(new debug_allocator_info{std::move(id)}) {}

    template <class U>
    debug_allocator(debug_allocator<U, POCC, POCCA, POCMA, POCS> const& other)
        : info(other.info) {}

    // Allocators move constructor must not actually move (call the copy
    // construct)
    template <class U>
    debug_allocator(debug_allocator<U, POCC, POCCA, POCMA, POCS>&& other)
        : debug_allocator(other) {}

    T* allocate(std::size_t n)
    {
        T* ret = static_cast<T*>(operator new (n*sizeof(value_type)));

        // Keep a record of this
        this->info->allocations[ret] = n;

        return ret;
    }

    void deallocate(T* p, std::size_t n) noexcept
    {
        assert(this->info->allocations.find(p) != std::cend(this->info->allocations));
        assert(this->info->allocations[p] == n);
        this->info->allocations.erase(p);
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
    return x.info == y.info;
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

    friend void swap(alloc_aware_impl& a, alloc_aware_impl& b)
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
            char, std::false_type, std::false_type, std::false_type, std::false_type>;

        alloc_aware<a> x = a("a1");
        assert(x.get_allocator().id() == "a1");

        alloc_aware<a> y = x;
        assert(y.get_allocator().id() == "default");

        alloc_aware<a> z = alloc_aware<a>(x, a("a2"));
        assert(z.get_allocator().id() == "a2");
    }
    {
        using a = debug_allocator<
            char, std::true_type, std::false_type, std::false_type, std::false_type>;

        alloc_aware<a> x = a("a1");
        alloc_aware<a> y = x;
        assert(y.get_allocator().id() == "a1");
    }
}

template<template<class> class alloc_aware>
void test_move_construct()
{
    using a = debug_allocator<
        char, std::false_type, std::false_type, std::false_type, std::false_type>;

    alloc_aware<a> x = a("a1");
    assert(x.get_allocator().id() == "a1");

    alloc_aware<a> y = std::move(x);
    assert(y.get_allocator().id() == "a1");

    x = a("a1");
    alloc_aware<a> z = alloc_aware<a>(std::move(x), a("a2"));
    assert(z.get_allocator().id() == "a2");
}

template<template<class> class alloc_aware>
void test_copy_assign()
{
    {
        using a = debug_allocator<
            char, std::false_type, std::false_type, std::false_type, std::false_type>;

        alloc_aware<a> x = a("a1");
        alloc_aware<a> y = a("a2");

        x = y;
        assert(x.get_allocator().id() == "a1");
    }
    {
        using a = debug_allocator<
            char, std::false_type, std::true_type, std::false_type, std::false_type>;

        alloc_aware<a> x = a("a1");
        alloc_aware<a> y = a("a2");

        x = y;
        assert(x.get_allocator().id() == "a2");
    }
}

template<template<class> class alloc_aware>
void test_move_assign()
{
    {
        using a = debug_allocator<
            char, std::false_type, std::false_type, std::false_type, std::false_type>;

        alloc_aware<a> x = a("a1");
        alloc_aware<a> y = a("a2");

        x = std::move(y);
        assert(x.get_allocator().id() == "a1");
    }
    {
        using a = debug_allocator<
            char, std::false_type, std::false_type, std::true_type, std::false_type>;

        alloc_aware<a> x = a("a1");
        alloc_aware<a> y = a("a2");

        x = std::move(y);
        assert(x.get_allocator().id() == "a2");
    }
    {
        using a = debug_allocator<
            char, std::false_type, std::false_type, std::false_type, std::false_type>;

        a alloc = a("a1");
        alloc_aware<a> x = alloc;
        alloc_aware<a> y = alloc;

        x = std::move(y);
        assert(x.get_allocator().id() == "a1");
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
}


