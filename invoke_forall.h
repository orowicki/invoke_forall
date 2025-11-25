#ifndef INVOKE_FORALL_H
#define INVOKE_FORALL_H

#include <array>
#include <concepts>
#include <cstddef>
#include <functional>
#include <tuple>
#include <type_traits>
#include <utility>

/*
na razie bez tego

using std::array;
using std::forward;
using std::get;
using std::index_sequence;
using std::invoke;
using std::is_void_v;
using std::make_index_sequence;
using std::remove_cvref_t;
using std::same_as;
using std::size_t;
using std::tuple_size;
using std::tuple_size_v;
*/

namespace detail {

    template <typename T>
    concept TupleLike = requires {
        typename std::tuple_size<T>;
    };

    template <typename T>
    concept Gettable = TupleLike<T> && requires(T t) {
        []<std::size_t... I>(std::index_sequence<I...>, T& x) {
            (get<I>(x), ...);
        }(std::make_index_sequence<std::tuple_size_v<std::remove_cvref_t<T>>>{}, t);
    };

    template <std::size_t I, typename T>
    constexpr decltype(auto) try_get(T&& t) {
        if constexpr (Gettable<T>)
            return std::get<I>(std::forward<T>(t));
        else
            return std::forward<T>(t);
    }

    template <std::size_t... I, typename... Args>
    constexpr decltype(auto) invoke_helper(std::index_sequence<I...>, Args&&... args) {
        return std::tuple{std::invoke(try_get<I>(std::forward<Args>(args))...)...};
    }

    /*
    bylo na labach, moze sie przyda (tego nie ma w scenariuszu):

    template <typename F, typename A, typename... Args>
    constexpr decltype(auto) invoke_helper(F&& f, A&& a, Args&&... args)
    {
        if constexpr (is_void_v<decltype(invoke(forward<F>(f)), forward<A>(a))>) {
            invoke(forward<F>(f), forward<A>(a));
            invoke_helper(forward<F>(f), forward<Args>(args)...);
        } else {
            return array{invoke(forward<F>(f), forward<A>(a)),
                         invoke(forward<F>(f), forward<Args>(args))...};
        }
    }
    */

    // TODO: implement
    // template <typename... Args>
    // concept SameArity = 

    template <typename F, typename A, typename... Args>
    constexpr decltype(auto) invoke_forall_helper(F&& f, A&& a, Args&&... args)
    {

    }

} /* namespace detail */

template <typename... Args> 
constexpr decltype(auto) invoke_forall(Args&&... args)
{
    return detail::invoke_forall_helper(std::forward<Args>(args));
}

template <typename T> 
constexpr decltype(auto) protect_arg(T&&)
{
    
}

#endif /* INVOKE_FORALL_H */
