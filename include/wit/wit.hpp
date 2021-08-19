#ifndef WIT_WIT_HPP
#define WIT_WIT_HPP
#include <memory>
#include <type_traits>
#include <tuple>
#include <functional>

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

    template<int index>
    struct placeholder {};
}
namespace std
{
    template<int index>
    struct is_placeholder<wit::placeholder<index>>
        : integral_constant<int, index> {};
}
namespace wit
{
    // TODO: Perfect forwarding
    template<typename host_t, typename... T>
    host_t create(T... t)
    {
        return host_t(t...);
    }

    template<typename host_t, typename int_t, int_t... ints, typename... T>
    auto create_helper(std::integer_sequence<int_t, ints...>, T... ts)
    {
        auto cimpl = create<host_t, std::allocator_arg_t,
             typename std::tuple_element<(ints+1)%sizeof...(ints),std::tuple<T...>>::type...>;
        auto x = std::bind(cimpl, std::allocator_arg,
                placeholder<((ints + 1) % sizeof...(ints))+1>{}...);

        return x(ts...);
    }

    template<typename host_t>
    struct wit : host_t
    {
        using allocator_type = typename host_t::allocator_type;

        template<typename... T>
        wit(std::allocator_arg_t, allocator_type a, T&&... t) :
            host_t(std::allocator_arg, a, std::forward<T>(t)...) {}

        template<typename... T,
            typename V=std::enable_if_t<
                std::is_same<get_last_t<T...>, allocator_type>::value
            >,
            typename U = V
        >
        wit(T&&... t)
            : host_t(create_helper<host_t>(std::make_index_sequence<sizeof...(T)>{}, t...)) {}

        template<typename... T,
            typename=std::enable_if_t<
                !std::is_same<get_last_t<T...>, allocator_type>::value
            >
        >
        wit(T&&... t) :
            host_t(std::allocator_arg, allocator_type(), std::forward<T>(t)...) {}
    };
}

#endif
