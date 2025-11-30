/**
 * Interface and implementation of the `invoke_forall` template.
 *
 * Authors: Adam Bęcłowicz, Oskar Rowicki
 * Date 30.11.2025
 *
 * The `invoke_forall` template allows the user to perform multiple
 * `std::invoke` calls. The function takes both regular and Gettable
 * arguments and returns an object that the user can use `std::get<i>` on to
 * get the result of the `i`-th invoke.
 *
 * The module also provides the `protect_arg()` function that makes
 * `invoke_forall` treat protected Gettable arguments as regular arguments.
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

namespace detail
{

template <typename T> 
struct protected_arg {
    T value;
    using is_protected_tag = void;

    explicit constexpr protected_arg(T&& arg) : value(std::forward<T>(arg)) {}
};

/** Satisfied if `T` is protected by `protect_arg()`. */
template <typename T>
concept Protected =
    requires { typename std::remove_cvref_t<T>::is_protected_tag; };

template <typename T>
concept HasTupleSize =
    requires { typename std::tuple_size<std::remove_cvref_t<T>>; };

template <typename T>
concept HasTupleSizeDerivedFromIntegralConstant =
    HasTupleSize<T> &&
    std::is_base_of_v<
        std::integral_constant<std::size_t,
                               std::tuple_size<std::remove_cvref_t<T>>::value>,
        std::tuple_size<std::remove_cvref_t<T>>>;

/**
 * Satisfied if:
 * - `T` has a specialization of `std::tuple_size<T>`, and
 * - that specialization derives from `std::integral_constant`.
 */
template <typename T>
concept TupleLike = HasTupleSizeDerivedFromIntegralConstant<T>;

/** Satisfied if `std::get<I>(t)` is valid. */
template <std::size_t I, typename T>
concept HasGet = requires(T t) { (void)std::get<I>(t); };

/**
 * Satisfied if:
 * - `T` is TupleLike and not protected by `protect_arg()`, and
 * - `std::get<i>(t)` is valid for all indices `0 ≤ i < std::tuple_size_v<T>`.
 */
template <typename T>
concept Gettable =
    TupleLike<T> && !Protected<T> &&
    []<std::size_t... Is>(std::index_sequence<Is...>) {
        return (... && HasGet<Is, T>);
    }(std::make_index_sequence<std::tuple_size_v<std::remove_cvref_t<T>>>{});

template <typename... Args>
concept NoneGettable = (... && !Gettable<Args>);

template <typename... Args>
concept NonEmpty = sizeof...(Args) > 0;

/**
 * Returns the arity of the first Gettable argument,
 * or `0` if none are found.
 */
template <typename T, typename... Args>
constexpr std::size_t first_arity_or_zero()
{
    if constexpr (Gettable<T>) {
        return std::tuple_size_v<std::remove_cvref_t<T>>;
    } else if constexpr (NonEmpty<Args...>) {
        return first_arity_or_zero<Args...>();
    } else {
        return 0;
    }
}

/**
 * Satisfied if `T` is either:
 * - non-Gettable, or
 * - has arity equal to `A`.
 */
template <std::size_t A, typename T>
concept HasArity =
    !Gettable<T> || std::tuple_size_v<std::remove_cvref_t<T>> == A;

/** Satisfied if all arguments have the same arity, equal to `A`. */
template <std::size_t A, typename... Args>
concept HaveArity = (... && HasArity<A, Args>);

/* Satisfied if all arguments have the same arity. */
template <typename... Args>
concept SameArity = HaveArity<first_arity_or_zero<Args...>(), Args...>;

/**
 * Tries to forward the given value `t`.
 *
 * If `t` is an non-Gettable rvalue reference (`T&&`), then it is moved only
 * during the last invoke, i.e. when `A == I + 1`.
 */
template <std::size_t A, std::size_t I, typename T>
constexpr decltype(auto) forward_copy_rvalue(T&& t)
{
    if constexpr (A != I + 1 && !Gettable<T> && !Protected<T> &&
                  std::is_rvalue_reference_v<T&&>) {
        return std::remove_cvref_t<T>(t);
    } else {
        return std::forward<T>(t);
    }
}

/**
 * Returns the appropriate value based on `T`:
 * - if `T` is Gettable, returns the `I`-th element of `t`
 * - if `T` is Protected, extracts its value from the wrapper and returns it
 * - otherwise returns `t` as it is
 */
template <std::size_t I, typename T> 
constexpr decltype(auto) try_get(T&& t)
{
    if constexpr (Gettable<T>) {
        return std::get<I>(std::forward<T>(t));
    } else if constexpr (Protected<T>) {
        auto&& value = std::forward<T>(t).value;
        return std::forward<decltype(value)>(value);
    } else {
        return std::forward<T>(t);
    }
}

/**
 * Performs the `I`-th `std::invoke(x1, ..., xn)` with:
 * - `xi = std::get<I>(argi)` if `argi` is Gettable, or
 * - `xi = argi`.
 *
 * If the return type of the invoke is `void`, performs the call, but returns
 * `std::monostate`, so that the result of `invoke_forall` isn't broken.
 */
template <std::size_t I, typename... Args>
constexpr decltype(auto) invoke_at(Args&&...args)
{
    auto call_invoke = [&]() -> decltype(auto) {
        return std::invoke(try_get<I>(std::forward<Args>(args))...);
    };

    if constexpr (std::is_void_v<decltype(call_invoke())>) {
        call_invoke();
        return std::monostate{};
    } else {
        return call_invoke();
    }
}

/**
 * Serves as a `invoke_at()` wrapper that forwards its arguments
 * through `forward_copy_rvalue()`.
 */
template <std::size_t A, std::size_t I, typename... Args>
constexpr decltype(auto) invoke_at_wrapper(Args&&...args)
{
    return invoke_at<I>(forward_copy_rvalue<A, I>(std::forward<Args>(args))...);
}

/**
 * Custom container that holds lvalue references and satisfies
 * `std::ranges::random_access_range`.
 * This is needed in case `invoke_forall` returns the same
 * lvalue reference type for every `std::invoke`, because `std::array` can not
 * hold lvalue references.
 */
template <typename T, std::size_t N> 
struct ref_range {
    std::array<T *, N> ptrs;

    template <typename... Args>
    constexpr ref_range(Args&&...args) : ptrs{ &args... } {}

    /**
     * Custom iterator that fits the random access iterator category.
     */
    struct iterator {
        using iterator_category = std::random_access_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = T;
        using pointer = T*;
        using reference = T&;

        T **ptr;

        constexpr reference operator*() const { return **ptr; }
        constexpr pointer operator->() const { return *ptr; }

        constexpr iterator& operator++()
        {
            ++ptr;
            return *this;
        }

        constexpr iterator operator++(int)
        {
            iterator tmp = *this;
            ++ptr;
            return tmp;
        }

        constexpr iterator& operator--()
        {
            --ptr;
            return *this;
        }

        constexpr iterator operator--(int)
        {
            iterator tmp = *this;
            --ptr;
            return tmp;
        }

        constexpr iterator& operator+=(difference_type n)
        {
            ptr += n;
            return *this;
        }
        constexpr iterator& operator-=(difference_type n)
        {
            ptr -= n;
            return *this;
        }

        friend constexpr iterator operator+(iterator it, difference_type n)
        {
            return { it.ptr + n };
        }

        friend constexpr iterator operator+(difference_type n, iterator it)
        {
            return { it.ptr + n };
        }

        friend constexpr iterator operator-(iterator it, difference_type n)
        {
            return { it.ptr - n };
        }

        friend constexpr difference_type operator-(iterator a, iterator b)
        {
            return a.ptr - b.ptr;
        }

        constexpr auto operator<=>(const iterator& other) const = default;

        constexpr reference operator[](difference_type n) const
        {
            return *ptr[n];
        }
    };

    constexpr iterator begin() const
    {
        return { const_cast<T **>(ptrs.data()) };
    }
    constexpr iterator end() const
    {
        return { const_cast<T **>(ptrs.data()) + N };
    }

    constexpr std::size_t size() const { return N; }
    constexpr T& operator[](std::size_t i) const { return *ptrs[i]; }

    template <std::size_t I>
    constexpr T& get() const { return *ptrs[I]; }
};

} /* namespace detail */

/**
 * std injection that allows for `std::get<i>()` on the custom ref_range 
 * container - makes ref_range tuple-like.
 */
namespace std
{

template <typename T, size_t N>
struct tuple_size<detail::ref_range<T, N>> : integral_constant<size_t, N> {
};

template <size_t I, typename T, size_t N>
struct tuple_element<I, detail::ref_range<T, N>> {
    using type = T&;
};

template <size_t I, typename T, size_t N>
constexpr T& get(const detail::ref_range<T, N>& r) noexcept
{
    return r.template get<I>();
}

} /* namespace std */

namespace detail
{

/**
 * Sequentially does `m` invoke calls, where `m` is the common arity of all
 * Gettable arguments.
 *
 * If each call results in the same return type, returns a container that
 * satisfies the `std::ranges::random_access_range` concept.
 */
template <std::size_t... Is, typename... Args>
constexpr decltype(auto) invoke_for_all_indices(std::index_sequence<Is...>,
                                                Args&&...args)
{
    constexpr size_t arity = sizeof...(Is);

    using first_result_type =
        decltype(invoke_at_wrapper<arity, 0>(std::forward<Args>(args)...));

    if constexpr ((... && std::same_as<first_result_type,
                                       decltype(invoke_at_wrapper<arity, Is>(
                                           std::forward<Args>(args)...))>)) {
        if constexpr (std::is_reference_v<first_result_type>) {
            using base_type = std::remove_reference_t<first_result_type>;

            return ref_range<base_type, arity>{
                invoke_at_wrapper<arity, Is>(std::forward<Args>(args)...)... 
            };
        } else {
            return std::array<first_result_type, arity>{
                invoke_at_wrapper<arity, Is>(std::forward<Args>(args)...)...
            };
        }
    } else {
        return std::tuple{ 
            invoke_at_wrapper<arity, Is>(std::forward<Args>(args)...)...
        };
    }
}

/**
 * If none of the arguments `(arg1, ..., argn)` are Gettable,
 * returns a result equivalent to calling `std::invoke(arg1, ..., argn)`.
 *
 * Otherwise, the return value `ret` is an object such that
 * `std::get<i>(ret)` is the result of the `i`-th invoke.
 */
template <typename... Args>
requires NonEmpty<Args...> && SameArity<Args...>
constexpr decltype(auto) invoke_forall(Args&&...args)
{
    if constexpr (NoneGettable<Args...>) {
        return invoke_at<0>(std::forward<Args>(args)...);
    } else {
        constexpr size_t arity = first_arity_or_zero<Args...>();

        return invoke_for_all_indices(std::make_index_sequence<arity>{},
                                      std::forward<Args>(args)...);
    }
}

/**
 * Makes `invoke_forall` treat protected Gettable argument `arg` as a regular
 * argument.
 *
 * If given argument is non-Gettable, does nothing and returns `arg` as it is.
 */
template <typename T> 
constexpr decltype(auto) protect_arg(T&& arg)
{
    if constexpr (Gettable<T>) {
        return protected_arg<T>{ std::forward<T>(arg) };
    } else {
        return std::forward<T>(arg);
    }
}

} /* namespace detail */

template <typename... Args>
constexpr decltype(auto) invoke_forall(Args&&...args)
{
    return detail::invoke_forall(std::forward<Args>(args)...);
}

template <typename T> 
constexpr decltype(auto) protect_arg(T&& arg)
{
    return detail::protect_arg(std::forward<T>(arg));
}

#endif /* INVOKE_FORALL_H */
