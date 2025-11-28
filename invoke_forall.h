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
using std::is_void_v;
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

template <typename... Args>
concept NoneGettable = (... && !Gettable<Args>);

/* -------------------------------------------------------------------------- */

template <size_t A, size_t I, typename T>
constexpr decltype(auto) forward_move_once(T&& t) {
    if constexpr (A == I + 1) {
        return std::forward<T>(t);
    }
    else {
        if constexpr (!Gettable<T> && is_rvalue_reference_v<T&&>) {
            if constexpr (Protected<T>) {
                return std::forward<T>(t); // to powinno byc zmienione
                                           // na cos co tworzy deep-copy
            }
            else {
                return T(t);
            }
        }
        else {
            return std::forward<T>(t);
        }
    }
}

template <size_t I, typename T> 
constexpr decltype(auto) try_get(T&& t)
{
    if constexpr (Gettable<T>) {
        return get<I>(std::forward<T>(t));
    }
    else {
        if constexpr (Protected<T>) {
            auto&& value = std::forward<T>(t).value;
            return std::forward<decltype(value)>(value);
        }
        else {
            return std::forward<T>(t);
        }
    }
}

/* -------------------------------------------------------------------------- */

template <typename... Args>
concept AtLeastOneArg = sizeof...(Args) > 0;

// TODO: rewrite (?)
template <typename... Args>
constexpr size_t first_arity()
{
    size_t arity = 0;
    (([&] {
        if constexpr (Gettable<Args>) {
            if (arity == 0) {
                arity = tuple_size_v<remove_cvref_t<Args>>;
            }
        }
    }()), ...);
    return arity;
}

template <size_t A, typename T>
concept HasArity = !Gettable<T> || tuple_size_v<remove_cvref_t<T>> == A;

template <size_t A, typename... Args>
concept HaveArity = (... && HasArity<A, Args>);

template <typename... Args>
concept SameArity = HaveArity<first_arity<Args...>(), Args...>;

/* -------------------------------------------------------------------------- */

template <size_t I, typename... Args>
constexpr decltype(auto) invoke_at(Args&&... args)
{
    auto call_invoke = [&]() -> decltype(auto) {
        return invoke(try_get<I>(std::forward<Args>(args))...);
    };

    if constexpr (is_void_v<decltype(call_invoke())>) {
        call_invoke();
        return monostate{};
    }
    else {
        return call_invoke();
    }
}

template <size_t A, size_t I, typename... Args>
constexpr decltype(auto) invoke_at_helper(Args&&... args)
{
    return invoke_at<I>(forward_move_once<A, I>(std::forward<Args>(args))...);
}

// TODO: cosmetics
template <size_t... Is, typename... Args>
constexpr decltype(auto) invoke_forall_helper(index_sequence<Is...>, Args&&... args)
{
    constexpr size_t arity = sizeof...(Is);        
    
    using result_type = decltype(
        invoke_at_helper<arity, 0>(std::forward<Args>(args)...)
    );

    if constexpr ((... && same_as<result_type, decltype(invoke_at_helper<arity, Is>(std::forward<Args>(args)...))>)) {
        return array<result_type, arity>{
            invoke_at_helper<arity, Is>(std::forward<Args>(args)...)...
        };
    }
    else {
        return tuple{ 
            invoke_at_helper<arity, Is>(std::forward<Args>(args)...)...
        };
    }
}

template <typename... Args>
requires AtLeastOneArg<Args...> && SameArity<Args...>
constexpr decltype(auto) invoke_forall(Args&&... args) {
    if constexpr (NoneGettable<Args...>) {
        return invoke_at<0>(std::forward<Args>(args)...);
    }
    else {
        constexpr size_t arity = first_arity<Args...>();

        return invoke_forall_helper(
            make_index_sequence<arity>{},
            std::forward<Args>(args)...
        );
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
