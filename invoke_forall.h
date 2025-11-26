#ifndef INVOKE_FORALL_H
#define INVOKE_FORALL_H

#include <array>
#include <concepts>
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

namespace detail
{

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


/**
 * Concepts
 */
/*
template <typename T>
concept TupleLike = 
    requires { typename std::tuple_size<std::remove_cvref_t<T>>; };
*/
template <typename T, typename = void>
struct is_tuple_like_impl : std::false_type {};

template <typename T>
struct is_tuple_like_impl<T, std::void_t<typename std::tuple_size<std::remove_cvref_t<T>>::type>>
    : std::true_type {};

// concept
template <typename T>
concept TupleLike = is_tuple_like_impl<T>::value;


template <std::size_t I, typename T>
concept Gettable =
    TupleLike<T> && requires(T &&t) { std::get<I>(std::forward<T>(t)); };

template <typename T>
concept FullyGettable = !Protected<T> && TupleLike<T> && requires(T t) {
    []<std::size_t... I>(std::index_sequence<I...>, T & x) {
        (get<I>(x), ...);
    }(std::make_index_sequence<std::tuple_size_v<std::remove_cvref_t<T>>>{}, t);
};


template <std::size_t I, typename T> 
constexpr decltype(auto) try_get(T &&t)
{
    if constexpr (Protected<T>) {
        /*
         if constexpr (std::is_lvalue_reference_v<T>) {
            return std::forward<T>(t.value);
        } else {
            return t.value;
        }
         */
         return t.value;
    } else if constexpr (Gettable<I, T>) {
        return std::get<I>(std::forward<T>(t));
    } else {
        if constexpr (std::is_lvalue_reference_v<T>) {
            return std::forward<T>(t);
        } else {
            return t;
        }
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
    std::size_t result = 0;
    // fold expression over Args
    (([&] {
        if constexpr (!Protected<Args> && TupleLike<Args>) {
            if (result == 0) {
                result = std::tuple_size_v<std::remove_cvref_t<Args>>;
            }
        }
    }()), ...);
    return result;
}


/**
 * Satisfied if all unprotected TupleLike args from Args have arity equal
 * to the paramater.
 */
template <std::size_t arity, typename... Args>
concept SameArity =
    (((!Protected<Args> && TupleLike<Args>)
          ? std::tuple_size_v<std::remove_cvref_t<Args>> == arity
          : true) &&
     ...);

/**
 * Satisfied if there is at least one arg and if any args are tuplelike, 
 * they all have the same arity.
 */
template <typename... Args>
concept ValidArgs =
    (sizeof...(Args) > 0) && (find_first_arity<Args...>() == 0 ||
                              SameArity<find_first_arity<Args...>(), Args...>);

/**
 * Performs the I-th invoke on all args.
 */
template <std::size_t I, typename... Args>
constexpr auto invoke_at(Args &&...args)
{ 
    using return_type = decltype(std::invoke(try_get<I>(std::forward<Args>(args))...));
    if constexpr (std::same_as<void, return_type>) {
        std::invoke(try_get<I>(std::forward<Args>(args))...);
        return std::monostate{}; // placeholder for void
    } else {
        return std::invoke(try_get<I>(std::forward<Args>(args))...);
    }
}

/**
 * Performes invokes on all args for I = 0,1,... given by the index_sequence
 */
template <std::size_t... I, typename... Args>
constexpr decltype(auto) invoke_helper(std::index_sequence<I...>,
                                       Args &&...args)
{
    using first_invoke_type = decltype(invoke_at<0>(args...));

    if constexpr ((std::same_as<first_invoke_type,
                                decltype(invoke_at<I>(args...))> &&
                   ...)) {
        return std::array<first_invoke_type, sizeof...(I)>{ invoke_at<I>(
            std::forward<Args>(args)...)... };
    } else {
        return std::tuple{ invoke_at<I>(std::forward<Args>(args)...)... };
    }
}

} /* namespace detail */

template <typename... Args>
//requires detail::ValidArgs<Args...>
constexpr decltype(auto) invoke_forall(Args &&...args)
{
    if constexpr (((!detail::Protected<Args> && detail::TupleLike<Args>) || ...)) {
        constexpr size_t arity = detail::find_first_arity<Args...>();
        return detail::invoke_helper(std::make_index_sequence<arity>{},
                                     std::forward<Args>(args)...);
    } else {
        return detail::invoke_at<0>(std::forward<Args>(args)...);
    }
}

template <typename T> constexpr decltype(auto) protect_arg(T &&arg)
{
    return detail::protected_arg<T>{ std::forward<T>(arg) };
}

#endif /* INVOKE_FORALL_H */
