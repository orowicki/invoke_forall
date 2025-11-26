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

// na niektore using (np. using std::forward) krzyczy clang++, wiec zostaje std

using std::array;
using std::index_sequence;
using std::invoke;
using std::make_index_sequence;
using std::monostate;
using std::remove_cvref_t;
using std::same_as;
using std::size_t;
using std::tuple;
using std::tuple_size;
using std::tuple_size_v;

namespace detail
{

/**
 * Argument protecting logic
 */
template <typename T> 
struct protected_arg {
    T value;

    template <typename U>
    explicit constexpr protected_arg(U&& arg) : value(std::forward<U>(arg)) {}
};

template <typename T>
struct is_protected : std::false_type {};

template <typename T>
struct is_protected<protected_arg<T>> : std::true_type {};

template <typename T>
concept Protected = is_protected<std::remove_cvref_t<T>>::value;

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

/*
> Uwaga.
> Jeżeli argumenty wywołania invoke_forall implikują co najmniej dwukrotne
> wywołanie std::invoke i istnieje zwykły argument (niekrotkowy), który jest
> r-wartością, to należy pamiętać, że r-wartość można przekazać dalej tylko raz,
> więc w pozostałych wywołaniach niezbędne jest utworzenie kopii tego argumentu
> przed przekazaniem go do std::invoke. Zakładamy, że taki argument ma
> zdefiniowany zarówno konstruktor kopiujący, jak i przenoszący.

TODO: przemyslec
*/
template <std::size_t I, std::size_t A, typename T>
constexpr decltype(auto) forward_move_once(T&& t) {
    if constexpr (I + 1 == A)
        return std::forward<T>(t);
    else {
        if constexpr (std::is_rvalue_reference_v<T>)
            return T(t);
        else
            return t;
    }
}

template <std::size_t I, typename T> 
constexpr decltype(auto) try_get(T&& t)
{
    if constexpr (Protected<T>) {
        /*
        if constexpr (std::is_lvalue_reference_v<T>) {
            return std::forward<T>(t.value);
        } else {
            return t.value;
        }
         */
        return (std::forward<T>(t).value);
    } else if constexpr (Gettable<T>) {
        return std::get<I>(std::forward<T>(t));
    } else {
        return std::forward<T>(t);
    }
}

/**
 * Finds the arity of the first unprotected TupleLike argument.
 * If successful returns it, otherwise returns 0.
 */
/*
template <typename... Args>
constexpr std::size_t find_first_arity()
{
    std::size_t result = 0;
    (((result == 0 && !Protected<Args> && TupleLike<Args>)
          ? result = std::tuple_size_v<std::remove_cvref_t<Args>>
          : 0),
     ...);
    return result;
}
*/

template <typename... Args>
constexpr std::size_t find_first_arity()
{
    std::size_t result = 1;
    // fold expression over Args
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
    if constexpr (same_as<decltype(invoke(try_get<I>(args)...)), void>)
        return monostate{};
    else
        return invoke(try_get<I>(std::forward<Args>(args))...);
}

// TODO: forward_move_once()
// uwaga co do przenoszenia, okreslajac typ chcemy wysylac kopie, a przeniesc
// dopiero wolajac invoke_at (a jesli nie jest Gettable, to wolajac invoke_at
// dopiero przy najwiekszym indeksie w Is)
template <size_t... Is, typename... Args>
constexpr decltype(auto) invoke_forall_helper(index_sequence<Is...>, Args&&... args)
{
    using invoke_at0_type = decltype(invoke_at<0>(args...));
    constexpr size_t arity = sizeof...(Is);

    if constexpr ((... && same_as<invoke_at0_type, decltype(invoke_at<Is>(args...))>))
        return array<invoke_at0_type, arity>{invoke_at<Is>(args...)...}; // tu powinno byc forward_move_once()
    else
        return tuple{invoke_at<Is>(args...)...}; // i tu tez
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
