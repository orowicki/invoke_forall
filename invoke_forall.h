/**
 * Interface and implementation of the invoke_forall template.
 *
 * Authors: Adam Bęcłowicz, Oskar Rowicki
 * Date 28.11.2025
 *
 * The invoke_forall template allows the user to perform multiple
 * std::invoke calls. The function takes both regular and tuple-like arguments 
 * and returns an object that the user can use get<I> on to get the result of the
 * I-th invoke.
 *
 * The module also provides the protect_arg() function that makes invoke_forall 
 * treat the protected tuple-like argument as a regular arg.
 */

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

/** Hidden implementation details. */
namespace detail
{

using std::array;
using std::get;
using std::index_sequence;
using std::integral_constant;
using std::invoke;
using std::is_base_of_v;
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

/** Argument protecting logic */
template <typename T> 
struct protected_arg {
    T value;
    using is_protected = void;

    explicit constexpr protected_arg(T &&arg) : value(std::forward<T>(arg)) {}
};

template <typename T>
concept Protected = requires(T t) { typename remove_cvref_t<T>::is_protected; };

/* -------------------------------------------------------------------------- */

/** T is TupleLike if std::tuple_size<T> is defined for it. */
template <typename T>
concept HasTupleSize = requires {
    typename tuple_size<remove_cvref_t<T>>;
};

template <typename T>
concept HasTupleSizeDerivedFromIntegralConstant = HasTupleSize<T> &&
    is_base_of_v<
        integral_constant<size_t, tuple_size<remove_cvref_t<T>>::value>,
        tuple_size<remove_cvref_t<T>>
    >;

template <typename T>
concept HasTupleSizeValue = HasTupleSize<T> && requires {
    { tuple_size_v<remove_cvref_t<T>> };
};

template <typename T>
concept TupleLike = HasTupleSizeDerivedFromIntegralConstant<T> &&
                    HasTupleSize<T>;

/* -------------------------------------------------------------------------- */

/**
 * T is Gettable if it's TupleLike and std::get<I> is defined for every
 * index I = 0, ..., std::tuple_size_v<T> - 1
 */
template <typename T>
concept Gettable = TupleLike<T> && !Protected<T> && requires(T t) {
    []<size_t... Is>(index_sequence<Is...>, T & x) {
        ((void)get<Is>(x), ...);
    }(make_index_sequence<tuple_size_v<remove_cvref_t<T>>>{}, t);
};

template <typename... Args>
concept NoneGettable = (... && !Gettable<Args>);

/* -------------------------------------------------------------------------- */

template <size_t A, size_t I, typename T>
constexpr decltype(auto) forward_copy_rvalue(T &&t)
{
    if constexpr (A == I + 1) {
        return std::forward<T>(t);
    } else {
        if constexpr (!Gettable<T> && is_rvalue_reference_v<T &&>) {
            if constexpr (Protected<T>) {
                return std::forward<T>(t); // to powinno byc zmienione
                                           // na cos co tworzy deep-copy
            } else {
                return T(t);
            }
        } else {
            return std::forward<T>(t);
        }
    }
}

/** Returns std::get<I> of t if t is Gettable, otherwise returns t. */
template <size_t I, typename T> 
constexpr decltype(auto) try_get(T &&t)
{
    if constexpr (Gettable<T>) {
        return get<I>(std::forward<T>(t));
    } else if constexpr (Protected<T>) {
        auto &&value = std::forward<T>(t).value;
        return std::forward<decltype(value)>(value);
    } else {
        return std::forward<T>(t);
    }
}

/* -------------------------------------------------------------------------- */

template <typename... Args>
concept NonEmpty = sizeof...(Args) > 0;

/** Finds the arity of the first Gettable argument in Args. */
template <typename T, typename... Args>
constexpr size_t first_arity() {
    if constexpr (Gettable<T>) return tuple_size_v<remove_cvref_t<T>>;
    else if constexpr (sizeof...(Args) > 0) return first_arity<Args...>();
    else return 0;
}

/** Satisfied if T isn't Gettable or is Gettable and has arity equal to Arity. */
template <size_t Arity, typename T>
concept HasArity = !Gettable<T> || tuple_size_v<remove_cvref_t<T>> == Arity;

/** True if all Args have arity equal Arity. */
template <size_t Arity, typename... Args>
concept HaveArity = (... && HasArity<Arity, Args>);

/** True if all Args have the same arity. */
template <typename... Args>
concept SameArity = HaveArity<first_arity<Args...>(), Args...>;

/* -------------------------------------------------------------------------- */

/**
 * Calls invoke with the I-th elements of Args.
 * (if arg isn't Gettable, passes the arg instead)
 */
template <size_t I, typename... Args>
constexpr decltype(auto) invoke_at_simple(Args &&...args)
{
    auto call_invoke = [&]() -> decltype(auto) {
        return invoke(try_get<I>(std::forward<Args>(args))...);
    };

    if constexpr (is_void_v<decltype(call_invoke())>) {
        call_invoke();
        return monostate{};
    } else {
        return call_invoke();
    }
}


template <size_t A, size_t I, typename... Args>
constexpr decltype(auto) invoke_at(Args &&...args)
{
    return invoke_at_simple<I>(
        forward_copy_rvalue<A, I>(std::forward<Args>(args))...);
}

// TODO: cosmetics
template <size_t... Is, typename... Args>
constexpr decltype(auto) invoke_for_all_indices(index_sequence<Is...>,
                                                Args &&...args)
{
    constexpr size_t arity = sizeof...(Is);
    using result_type =
        decltype(invoke_at<arity, 0>(std::forward<Args>(args)...));

    if constexpr ((... &&
                   same_as<result_type, decltype(invoke_at<arity, Is>(
                                            std::forward<Args>(args)...))>)) {
        return array<result_type, arity>{ invoke_at<arity, Is>(
            std::forward<Args>(args)...)... };
    } else {
        return tuple{ invoke_at<arity, Is>(std::forward<Args>(args)...)... };
    }
}

template <typename... Args>
requires NonEmpty<Args...> && SameArity<Args...>
constexpr decltype(auto) invoke_forall(Args&&... args) {
    if constexpr (NoneGettable<Args...>) {
        return invoke_at_simple<0>(std::forward<Args>(args)...);
    } else {
        constexpr size_t arity = first_arity<Args...>();

        return invoke_for_all_indices(make_index_sequence<arity>{},
                                      std::forward<Args>(args)...);
    }
}

} /* namespace detail */

template <typename... Args>
constexpr decltype(auto) invoke_forall(Args &&...args)
{
    return detail::invoke_forall(std::forward<Args>(args)...);
}

template <typename T> 
constexpr decltype(auto) protect_arg(T &&arg)
{
    return detail::protected_arg<T>{ std::forward<T>(arg) };
}

#endif /* INVOKE_FORALL_H */
