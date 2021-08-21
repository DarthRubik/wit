#ifndef WIT_WIT_HPP
#define WIT_WIT_HPP
#include <memory>
#include <type_traits>
#include <tuple>

namespace wit
{

    template<typename... Ts>
    struct get_last
    {
        using tup = std::tuple<Ts...>;
        using type = typename std::tuple_element<sizeof...(Ts) - 1, tup>::type;
    };

    template<>
    struct get_last<>
    {
        using type = void;
    };

    template<typename... T>
    using get_last_t = typename get_last<T...>::type;

    template<typename... Ts>
    struct get_first {};

    template<typename F, typename... Rest>
    struct get_first<F, Rest...> { using type = F; };

    template<>
    struct get_first<> { using type = void; };

    template<typename... T>
    using get_first_t = typename get_first<T...>::type;
}
namespace wit
{
    template<std::size_t index, typename... T>
    auto get_from_pack(T&&... t)
    {
        return std::get<(index+1)%sizeof...(t)>(
                std::make_tuple(t...));
    }

    template<typename host_t>
    struct wit : host_t
    {
        using allocator_type = typename host_t::allocator_type;

    private:
        using traits = std::allocator_traits<allocator_type>;

    public:

        wit(wit const& other)
            : host_t(other,
                traits::select_on_container_copy_construction(other.get_allocator()))
        {}

        wit(wit const& other, allocator_type const& a)
            : host_t(other, a)
        {}

        wit(wit&& other) : wit()
        {
            swap(static_cast<host_t&>(*this), static_cast<host_t&>(other));
        }

        wit(wit&& other, allocator_type const& a)
            : wit(a == other.get_allocator()
                    ? std::move(other) : wit(other, a)) {}

        static allocator_type p_alloc(wit const& a, wit const& b, std::true_type)
        {
            return a.get_allocator();
        }
        static allocator_type p_alloc(wit const& a, wit const& b, std::false_type)
        {
            return b.get_allocator();
        }

        wit& operator=(wit const& other)
        {
            host_t copy(other,
                p_alloc(other,*this,typename traits::propagate_on_container_copy_assignment{}));
            swap(static_cast<host_t&>(*this), copy);

            return *this;
        }

        wit& operator=(wit&& other)
        {
            swap(static_cast<host_t&>(*this), static_cast<host_t&>(other));

            return *this;
        }


        template<std::size_t... I, typename... T>
        wit(std::integer_sequence<std::size_t, I...>, T&&... t)
            : host_t(std::allocator_arg, get_from_pack<I>(std::forward<T>(t)...)...) {}

        template<typename... T>
        wit(std::allocator_arg_t, allocator_type a, T&&... t) :
            host_t(std::allocator_arg, a, std::forward<T>(t)...) {}

        template<typename... T,
            typename V=std::enable_if_t<
                std::is_same<std::decay_t<get_last_t<T...>>, allocator_type>::value &&
                !std::is_same<std::decay_t<get_first_t<T...>>, wit>::value
            >,
            typename U = V
        >
        wit(T&&... t)
            : wit(std::make_index_sequence<sizeof...(T)>{}, std::forward<T>(t)...) {}

        template<typename... T,
            typename=std::enable_if_t<
                !std::is_same<std::decay_t<get_last_t<T...>>, allocator_type>::value &&
                !std::is_same<std::decay_t<get_first_t<T...>>, wit>::value
            >
        >
        wit(T&&... t) :
            host_t(std::allocator_arg, allocator_type(), std::forward<T>(t)...) {}
    };
}

#endif
