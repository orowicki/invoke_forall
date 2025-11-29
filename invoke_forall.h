/**
 * Interface and implementation of the `invoke_forall` template.
 *
 * Authors: Adam Bęcłowicz, Oskar Rowicki
 * Date 29.11.2025
 *
 * The `invoke_forall` template allows the user to perform multiple
 * `std::invoke` calls. The function takes both regular and *tuple-like*
 * arguments and returns an object that the user can use `std::get<i>` on to
 * get the result of the `i`-th invoke.
 *
 * The module also provides the `protect_arg()` function that makes
 * `invoke_forall` treat the protected *tuple-like* argument as a regular arg.
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

/** Argument protecting logic */
template <typename T> 
struct protected_arg {
    T value;
    using is_protected = void;

    explicit constexpr protected_arg(T &&arg)
        : value(std::forward<T>(arg)) {}
};

template <typename T>
concept Protected = requires(T t) {
    typename std::remove_cvref_t<T>::is_protected;
};

/* -------------------------------------------------------------------------- */

template <typename T>
concept HasTupleSize = requires {
    typename std::tuple_size<std::remove_cvref_t<T>>;
};

template <typename T>
concept HasTupleSizeDerivedFromIntegralConstant = HasTupleSize<T> &&
    std::is_base_of_v<
        std::integral_constant<
            std::size_t, std::tuple_size<std::remove_cvref_t<T>>::value>,
        std::tuple_size<std::remove_cvref_t<T>>>;

/**
 * Concept `TupleLike<T>` is satisfied if:
 * - `T` has a specialization of `std::tuple_size<T>`, and
 * - that specialization derives from `std::integral_constant`.
 */
template <typename T>
concept TupleLike = HasTupleSizeDerivedFromIntegralConstant<T>;

/* -------------------------------------------------------------------------- */

template <std::size_t I, typename T>
concept HasGet = requires(T t) {
    (void)std::get<I>(t);
};

/**
 * Concept `Gettable<T>` is satisfied if:
 * - `T` satisfies the concept `TupleLike<T>`, and
 * - `T` is not protected by `protect_arg()`, and
 * - `std::get<i>(T)` is valid for all indices `0 ≤ i < std::tuple_size_v<T>`.
 */
template <typename T>
concept Gettable = TupleLike<T> && !Protected<T> &&
    []<std::size_t... Is>(std::index_sequence<Is...>) {
        return (... && HasGet<Is, T>);
    }(std::make_index_sequence<std::tuple_size_v<std::remove_cvref_t<T>>>{});

template <typename... Args>
concept NoneGettable = (... && !Gettable<Args>);

/* -------------------------------------------------------------------------- */

// TODO: document
template <std::size_t A, std::size_t I, typename T>
constexpr decltype(auto) forward_copy_rvalue(T&& t)
{
    if constexpr (A == I + 1) {
        return std::forward<T>(t);
    }
    else {
        if constexpr (!Gettable<T> && std::is_rvalue_reference_v<T&&>) {
            if constexpr (Protected<T>) {
                return std::forward<T>(t);
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

/**
 * Returns:
 * - `std::get<I>(t)` if `t` is *gettable*
 * - `t.value` if `t` is protected by `protect_arg()`
 * - `t`, otherwise
 */
template <std::size_t I, typename T> 
constexpr decltype(auto) try_get(T&& t)
{
    if constexpr (Gettable<T>) {
        return std::get<I>(std::forward<T>(t));
    }
    else if constexpr (Protected<T>) {
        auto &&value = std::forward<T>(t).value;
        return std::forward<decltype(value)>(value);
    }
    else {
        return std::forward<T>(t);
    }
}

/* -------------------------------------------------------------------------- */

template <typename... Args>
concept NonEmpty = sizeof...(Args) > 0;

/// Finds the arity of the first *gettable* argument.
template <typename T, typename... Args>
constexpr std::size_t first_arity_or_zero() {
    if constexpr (Gettable<T>) {
        return std::tuple_size_v<std::remove_cvref_t<T>>;
    }
    else if constexpr (sizeof...(Args) > 0) {
        return first_arity_or_zero<Args...>();
    }
    else {
        return 0;
    }
}

/**
 * Satisfied if:
 * - `T` is not *gettable*
 * - `T` is *gettable* and has arity equal to `A`
 */
template <std::size_t A, typename T>
concept HasArity = !Gettable<T> ||
                   std::tuple_size_v<std::remove_cvref_t<T>> == A;

/// `True` if all arguments have arity equal to `A`.
template <std::size_t A, typename... Args>
concept HaveArity = (... && HasArity<A, Args>);

/// `True` if all arguments have the same arity.
template <typename... Args>
concept SameArity = HaveArity<first_arity_or_zero<Args...>(), Args...>;

/* -------------------------------------------------------------------------- */

/**
 * Performs the `I`-th `std::invoke` with:
 * - `std::get<I>` applied to the arguments that satisfy `Gettable<T>`, or
 * - original arguments
 */
template <std::size_t I, typename... Args>
constexpr decltype(auto) invoke_at_simple(Args&&... args)
{
    auto call_invoke = [&]() -> decltype(auto) {
        return std::invoke(try_get<I>(std::forward<Args>(args))...);
    };

    if constexpr (std::is_void_v<decltype(call_invoke())>) {
        call_invoke();
        return std::monostate{};
    }
    else {
        return call_invoke();
    }
}

template <std::size_t A, std::size_t I, typename... Args>
constexpr decltype(auto) invoke_at(Args&&... args)
{
    return invoke_at_simple<I>(
        forward_copy_rvalue<A, I>(std::forward<Args>(args))...
    );
}

template <std::size_t... Is, typename... Args>
constexpr decltype(auto) invoke_for_all_indices(std::index_sequence<Is...>,
                                                Args&&... args)
{
    constexpr size_t arity = sizeof...(Is);        

    using result_type = decltype(
        invoke_at<arity, 0>(std::forward<Args>(args)...)
    );

    if constexpr ((... && std::same_as<result_type,
        decltype(invoke_at<arity, Is>(std::forward<Args>(args)...))
    >)) {
        return std::array<result_type, arity>{
            invoke_at<arity, Is>(std::forward<Args>(args)...)...
        };
    }
    else {
        return std::tuple{
            invoke_at<arity, Is>(std::forward<Args>(args)...)...
        };
    }
}

template <typename... Args>
    requires NonEmpty<Args...> && SameArity<Args...>
constexpr decltype(auto) invoke_forall(Args&&... args) {
    if constexpr (NoneGettable<Args...>) {
        return invoke_at_simple<0>(std::forward<Args>(args)...);
    }
    else {
        constexpr size_t arity = first_arity_or_zero<Args...>();

        return invoke_for_all_indices(std::make_index_sequence<arity>{},
                                      std::forward<Args>(args)...);
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
    return detail::protected_arg<T>{ std::forward<T>(arg) };
}

#endif /* INVOKE_FORALL_H */
