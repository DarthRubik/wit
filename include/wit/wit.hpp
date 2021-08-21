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

    /*
     * This basically returns the item at the index rotated.....that is the
     * list:
     *
     * a b c d
     *
     * Becomes:
     *
     * d a b c
     *
     */
    template<std::size_t index, typename... T>
    auto rotate_pack(T&&... t)
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
            : host_t(
                traits::select_on_container_copy_construction(other.get_allocator()), other)
        {}

        wit(wit const& other, allocator_type const& a)
            : host_t(a, other)
        {}

        wit(wit&& other) : wit()
        {
            swap(static_cast<host_t&>(*this), static_cast<host_t&>(other));
        }

        wit(wit&& other, allocator_type const& a)
            : wit(a == other.get_allocator()
                    ? std::move(other) : wit(other, a)) {}

        /*
         * This lets us specialize based on which allocator we are supposed to
         * use
         */
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
            // Copy using the appropriate allocator
            host_t copy(
                p_alloc(other,*this,typename traits::propagate_on_container_copy_assignment{}),
                other);
            swap(static_cast<host_t&>(*this), copy);

            return *this;
        }

        template<
            typename cond = typename traits::propagate_on_container_move_assignment,
            typename = std::enable_if_t<cond::value>>
        wit& operator=(wit&& other)
        {
            // In the case where we are propagating the allocator on move, then
            // it is really simple....just swap the two objects
            swap(static_cast<host_t&>(*this), static_cast<host_t&>(other));

            return *this;
        }

        template<
            typename cond = typename traits::propagate_on_container_move_assignment,
            typename T = cond,
            typename = std::enable_if_t<!cond::value>>
        wit& operator=(wit&& other)
        {
            /*
             * In the case where we are not propagating the allocator, it is
             * more complicated:
             *
             * - If the allocators match, then we do a simple swap,
             * - Else we are forced to do a copy, so that on deallocate we
             *   still work
             *
             */
            if (other.get_allocator() == this->get_allocator())
            {
                swap(static_cast<host_t&>(*this), static_cast<host_t&>(other));
            }
            else
            {
                host_t copy(this->get_allocator(), other);
                swap(copy, static_cast<host_t&>(*this));
            }

            return *this;
        }

        template<std::size_t... I, typename... T>
        wit(std::integer_sequence<std::size_t, I...>, T&&... t)
            : host_t(rotate_pack<I>(std::forward<T>(t)...)...) {}

        template<typename... T>
        wit(std::allocator_arg_t, allocator_type a, T&&... t) :
            host_t(a, std::forward<T>(t)...) {}

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
            host_t(allocator_type(), std::forward<T>(t)...) {}
    };
}

#endif
