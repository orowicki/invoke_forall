#ifndef INVOKE_FORALL_H
#define INVOKE_FORALL_H

#include <array>
#include <concepts>
#include <cstddef>
#include <functional>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

namespace detail
{

using std::array;
using std::get;
using std::index_sequence;
using std::invoke;
using std::is_rvalue_reference_v;
using std::make_index_sequence;
using std::monostate;
using std::remove_cvref_t;
using std::same_as;
using std::size_t;
using std::tuple;
using std::tuple_size;
using std::tuple_size_v;

/**
 * Argument protecting logic
 */
template <typename T> 
struct protected_arg {
    T&& value;
    using is_protected = void;

    explicit constexpr protected_arg(T&& arg) : value(std::forward<T>(arg)) {}
};

template <typename T>
concept Protected = requires(T t) { typename remove_cvref_t<T>::is_protected; };

/* -------------------------------------------------------------------------- */

template <typename T>
concept TupleLike = requires {
    sizeof(tuple_size<remove_cvref_t<T>>);
} && requires {
    { tuple_size_v<remove_cvref_t<T>> };
};

template <typename T>
concept Gettable = TupleLike<T> && !Protected<T> && requires(T t) {
    []<size_t... Is>(index_sequence<Is...>, T& x) {
        ((void)get<Is>(x), ...);
    }(make_index_sequence<tuple_size_v<remove_cvref_t<T>>>{}, t);
};

template <size_t A, typename... Args>
concept SameArityAs = (... && (!Gettable<Args> || tuple_size_v<remove_cvref_t<Args>> == A));

template <typename... Args>
concept SameArity = SameArityAs<find_first_arity<Args...>(), Args...>;

template <typename... Args>
concept NoneGettable = (... && !Gettable<Args>);

/* -------------------------------------------------------------------------- */

template <size_t I, size_t A, typename T>
constexpr decltype(auto) forward_move_once(T&& t) {
    if constexpr (I + 1 == A)
        return std::forward<T>(t);
    else {
        if constexpr (is_rvalue_reference_v<T&&>) {  // TODO: przemyslec
            return T(t);                            // to tez
        } else {
            return std::forward<T>(t);
        }
    }
}

template <size_t I, typename T> 
constexpr decltype(auto) try_get(T&& t)
{
    if constexpr (Protected<T>) {
        auto&& value = std::forward<T>(t).value;
        return std::forward<decltype(value)>(value);
    } else if constexpr (Gettable<T>) {
        return get<I>(std::forward<T>(t));
    } else {
        return std::forward<T>(t);
    }
}

template <typename... Args>
constexpr std::size_t find_first_arity()
{
    std::size_t result = 1;
    (([&] {
        if constexpr (!Protected<Args> && TupleLike<Args>) {
            if (result == 1) {
                result = std::tuple_size_v<std::remove_cvref_t<Args>>;
            }
        }
    }()), ...);
    return result;
}

/* -------------------------------------------------------------------------- */

template <size_t I, typename... Args>
constexpr decltype(auto) invoke_at(Args&&... args)
{
    if constexpr (same_as<decltype(invoke(try_get<I>(args)...)), void>) {
        return monostate{};
    } else {
        return invoke(try_get<I>(std::forward<Args>(args))...);
    }
}

template <size_t... Is, typename... Args>
constexpr decltype(auto) invoke_forall_helper(index_sequence<Is...>, Args&&... args)
{
    using invoke_at0_type = decltype(invoke_at<0>(args...));                                // czy tutaj jakies forward_move_never() ?
    constexpr size_t arity = sizeof...(Is);

    if constexpr ((... && same_as<invoke_at0_type, decltype(invoke_at<Is>(args...))>))      // i tutaj tez?
        return array<invoke_at0_type, arity>{
            invoke_at<Is>(forward_move_once<Is, arity>(std::forward<Args>(args))...)...
        };
    else
        return tuple{
            invoke_at<Is>(forward_move_once<Is, arity>(std::forward<Args>(args))...)...
        };
}

template <typename F, typename... Args>
constexpr decltype(auto) invoke_forall(F&& f, Args&&... args) {
    if constexpr (NoneGettable<F, Args...>)
        return invoke_at<0>(std::forward<F>(f), std::forward<Args>(args)...);
    else {
        constexpr size_t arity = find_first_arity<F, Args...>();
        return invoke_forall_helper(make_index_sequence<arity>{}, std::forward<F>(f), std::forward<Args>(args)...);
    }
}

} /* namespace detail */

template <typename... Args>
constexpr decltype(auto) invoke_forall(Args&&... args)
{
    return detail::invoke_forall(std::forward<Args>(args)...);
}

template <typename T>
constexpr decltype(auto) protect_arg(T&& arg)
{
    return detail::protected_arg<T>{std::forward<T>(arg)};
}

#endif /* INVOKE_FORALL_H */
