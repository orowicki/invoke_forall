#include "invoke_forall.h"

#ifdef NDEBUG
#undef NDEBUG
#endif

#include <array>
#include <cassert>
#include <concepts>
#include <numeric>

#ifdef TESTING
#include <iostream>
#endif

#include <functional>
#include <ranges>
#include <string>
#include <tuple>
#include <variant>

#ifdef TESTING
#define TEST_NUM TESTING
#endif

namespace {
    template <typename A, typename T, std::size_t N>
    constexpr bool acmp(const A& a, const std::array<T, N>& b) {
        if constexpr (N == 0) {
            return a.begin() == a.end();
        } else {
            if (std::size_t(a.end() - a.begin()) != N)
                return false;
            size_t i = 0;
            for (const auto& x : a)
                if (x != b[i++])
                    return false;
            return true;
        }
    }

    template <typename A, typename B>
    constexpr bool tcmp(const A& a, const B& b) {
        return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            return (true && ... && (std::get<Is>(a) == std::get<Is>(b)));
        } (std::make_index_sequence<std::tuple_size_v<B>>());
    }

#if TEST_NUM == 101 || TEST_NUM == 401 || TEST_NUM == 601
    constexpr inline int add(int a, int b) {
        return a + b;
    }
#endif

    template <int n>
    struct F {
        int v;
        constexpr F(int v = n) : v(v) {}
        constexpr int f() const { return v; }
        constexpr int g(int x) const { return v + x; }
        constexpr int h(int a, int b) const { return a * v + b; }
        constexpr char s() const { return 'a' + v; }
        auto operator<=>(const F&) const = default;
    };

    struct Tracer {
        static constexpr int NO_PARENT = -1;
        static constexpr int NOT_OWNED = -2;
        static constexpr int DESTROYED = -3;
        static inline int id_counter = 0;
        int id, parent_id = NO_PARENT;

        template <typename T, typename U = std::remove_reference_t<T>>
        using store_t =
            std::conditional_t<
                std::is_lvalue_reference_v<T>,
                T,
                std::conditional_t<
                    std::is_array_v<U>,
                    std::add_pointer_t<std::remove_extent_t<U>>,
                    std::conditional_t<
                        std::is_function_v<U>,
                        std::add_pointer_t<U>,
                        U
                    >
                >
            >;

        Tracer() : id(id_counter++) {}

        Tracer(const Tracer& other) : id(id_counter++), parent_id(other.id) {}

        constexpr Tracer(Tracer&& other) noexcept : id(other.id), parent_id(other.parent_id) {
            other.parent_id = NOT_OWNED;
        }

        Tracer& operator=(const Tracer& other) {
            id = id_counter++;
            parent_id = other.id;
            return *this;
        }

        constexpr Tracer& operator=(Tracer&& other) noexcept {
            id = other.id;
            parent_id = other.parent_id;
            other.parent_id = NOT_OWNED;
            return *this;
        }

        constexpr ~Tracer() {
            parent_id = DESTROYED;
       }

        constexpr bool is(int id) const {
            return this->id == id && parent_id == NO_PARENT;
        }

        constexpr bool is(int id, int parent_id) const {
            return this->id == id && this->parent_id == parent_id;
        }

        constexpr bool is_moved() const {
            return parent_id == NOT_OWNED;
        }

        constexpr bool is_alive() const {
            return parent_id >= NO_PARENT;
        }

        constexpr decltype(auto) operator()() const {
            return *this;
        }

        template <typename T>
        constexpr store_t<T> operator()(T&& x) const {
            return std::forward<T>(x);
        }

        template <typename... Ts> requires (sizeof...(Ts) > 1)
        constexpr auto operator()(Ts&&... ts) const {
            return std::tuple<store_t<Ts>...>{std::forward<Ts>(ts)...};
        }

#ifdef TESTING
        friend inline std::ostream& operator<<(std::ostream& os, const Tracer& tr) {
            return os << "[id=" << tr.id << ", p=" << tr.parent_id << "]";
        }
#endif
    };
} // anonymous namespace

#if TEST_NUM == 402 || TEST_NUM == 403
namespace std {
    template <int n>
    struct tuple_size<F<n>> : std::integral_constant<std::size_t, std::size_t(n)> {};
} // namespace std
#endif

#if TEST_NUM < 400 || TEST_NUM >= 500
#define sassert assert
#else
#define sassert static_assert
#endif

int main() {
#if TEST_NUM == 101 || TEST_NUM == 401
    // Basic tests
    sassert(invoke_forall([] { return 42; }) == 42);
    sassert(invoke_forall(std::negate{}, 5) == -5);
    sassert(invoke_forall(std::plus{}, 2, 3) == 5);
    // Tuple and array arguments
    sassert(acmp(invoke_forall(std::negate{}, std::array{1}), std::array{-1}));
    sassert(acmp(invoke_forall(std::negate{}, std::array{1, 2}), std::array{-1, -2}));
    sassert(tcmp(invoke_forall(std::negate{}, std::tuple{1, 2.0}), std::tuple{-1, -2.0}));
    sassert(acmp(invoke_forall(std::plus{}, std::array{1}, std::array{2}), std::array{3}));
    sassert(acmp(invoke_forall(std::plus{}, std::array{1, 2}, 3), std::array{4, 5}));
    sassert(acmp(invoke_forall(std::plus{}, std::array{1, 2}, std::pair{3, 4}), std::array{4, 6}));
    sassert(acmp(invoke_forall(std::plus{}, 1, std::array{3, 4}), std::array{4, 5}));
    sassert(tcmp(invoke_forall(std::plus{}, std::tuple{1, 2.0}, std::array{3, 4}), std::tuple{4, 6.0}));
    sassert(tcmp(
        invoke_forall(
            []<typename... Ts>(Ts&&... ts) { return std::tuple{std::forward<Ts>(ts)...}; },
            0,
            std::array{1, 2, 3},
            std::tuple{std::string("a"), (void*)nullptr, 4.5},
            false),
        std::tuple{
            std::tuple{0, 1, std::string("a"), false},
            std::tuple{0, 2, (void*)nullptr, false},
            std::tuple{0, 3, 4.5, false}}))
        ;
    // std::variant argument
    sassert(acmp(
        invoke_forall(
            [](int i, auto v) { return i + std::get<0>(v); },
            std::array{1, 2, 3}, std::variant<int, const char *>{10}),
        std::array{11, 12, 13}));
    // std::ranges::subrange argument
    {
        constexpr std::array v{1, 2, 3};
        sassert(acmp(
            invoke_forall(
                [](auto i, int d) { return i[d]; },
                std::ranges::subrange{v}, std::array{1, -1}),
            std::array{2, 3}));
    }
    // array of function objects
    sassert(acmp(
        invoke_forall(std::tuple{[]{ return 1; }, []{ return 2; }, []{ return 3; }}),
        std::array{1, 2, 3}));
    sassert(tcmp(
        invoke_forall(
            std::tuple{
                [](int x) { return ++x; },
                [](int x) { return 'a' + x; },
                [](int x) { return std::pair{x, x % 5}; }},
            16),
        std::tuple{17, 'q', std::pair{16, 1}}));
    sassert(acmp(
        invoke_forall(
            std::tuple{std::minus{}, &add, [](int x, int y) { return std::min(x, y);  }},
            std::array{20, 10, 0},
            std::array{5, 10, 15}),
        std::array{15, 20, 0}));
    // member function pointers and member data pointers
    sassert(acmp(
        invoke_forall(&F<2>::f,
            std::array{F<2>{1}, F<2>{2}, F<2>{3}}),
        std::array{1, 2, 3}));
    sassert(acmp(
        invoke_forall(
            std::tuple{&F<2>::f, &F<3>::f, &F<4>::f},
            std::tuple{F<2>{}, F<3>{}, F<4>{}}),
        std::array{2, 3, 4}));
    sassert(tcmp(
        invoke_forall(
            std::tuple{&F<2>::f, &F<2>::s},
            F<2>{}),
        std::tuple{2, 'c'}));
    sassert(acmp(
        invoke_forall(&F<2>::g,
            std::array{F<2>{1}, F<2>{2}, F<2>{3}},
            10),
        std::array{11, 12, 13}));
    sassert(acmp(
        invoke_forall(
            std::tuple{&F<2>::g, &F<3>::g, &F<4>::g},
            std::tuple{F<2>{}, F<3>{}, F<4>{}},
            20),
        std::array{22, 23, 24}));
    sassert(acmp(
        invoke_forall(
            std::tuple{&F<2>::h, &F<3>::h},
            std::tuple{F<2>{}, F<3>{}},
            std::array{5, 15},
            std::array{2, 4}),
        std::array{2 * 5 + 2, 3 * 15 + 4}));
#endif
#if TEST_NUM == 101
    // check condition for std::ranges::random_access_range
    {
        using namespace std::string_literals;
        std::string s;
        auto result =
            invoke_forall(
                [&s](const auto& arg) { return s.append(arg); },
                std::tuple{"a"s, "b"s, "c", std::string_view("d")});
        auto it = result.begin();
        assert(*it == "a");
        assert(it[1] == "ab");
        it += 2;
        assert(*it == "abc");
        assert(*(result.end() - 1) == "abcd");
    }
#endif
#if TEST_NUM == 102
    // argument required
    invoke_forall();
#endif
#if TEST_NUM == 103
    // argument mismatch
    invoke_forall([](int&) {}, 10);
#endif
#if TEST_NUM == 104
    // non-function object argument
    invoke_forall(std::array{"a", "b", "c"});
#endif
#if TEST_NUM == 105
    // array of function objects with different argument numbers
    invoke_forall(std::tuple{std::negate{}, std::plus<int>{}}, 10, 20);
#endif
#if TEST_NUM == 106
    // result is not std::ranges::random_access_range
    (void)acmp(invoke_forall(std::plus{}, std::tuple{1, 2.0}, std::array{3, 4}), std::array{4.0, 6.0});
#endif
#if TEST_NUM == 107
    // non-matching arities
    (void)invoke_forall(std::plus{}, std::array{1, 2, 3}, std::pair{4, 5});
#endif
#if TEST_NUM == 201 || TEST_NUM == 401
    // arity zero - optional tests
    // sassert(acmp(invoke_forall(std::negate{}, std::array<int, 0>{}), std::array<int, 0>{}));
    // sassert(acmp(invoke_forall(std::plus{}, std::array<int, 0>{}, std::array<int, 0>{}), std::array<int, 0>{}));
    // arity one
    sassert(acmp(invoke_forall(std::negate{}, std::array{-1}), std::array{1}));
#endif
#if TEST_NUM == 201
    {
        int count = 0;
        // void result and reference arguments
        invoke_forall([](int& x) { ++x; }, std::pair<int&, int&>(count, count));
        assert(count == 2);
        std::array a{1, 2, 3};
        invoke_forall([](int& x, int& y) { ++x; ++y; }, a, count);
        assert(a == (std::array{2, 3, 4}));
        assert(count == 5);
        std::array b{7, 8, 9};
        invoke_forall([](int& x, const int& y) { x = y; }, a, b);
        assert(a == b);
        // reference result
        auto get_ref = [](int& x) -> int& { return x; };
        {
            auto res = invoke_forall(std::array{get_ref, get_ref, get_ref}, count);
            for (auto& r : res)
                r++;
            assert(count == 8);
        }
        {
            auto res = invoke_forall(get_ref, b);
            for (auto& r : res)
                r++;
        }
        assert(b == (std::array{8, 9, 10}));
    }
#endif
#if TEST_NUM >= 201 && TEST_NUM <= 203
    {
        int count = 0;
        // reference result with const and mutable elements
        struct S {
            int s = 10;
        } s;
        auto res = invoke_forall(
            std::tuple{
                [](auto& x) -> int& { ++x; return x; },
                [](const auto& x) -> const int& { return x.s; }},
            std::forward_as_tuple(count, s));
        std::get<0>(res)++;
        assert(&std::get<0>(res) == &count);
        assert(&std::get<1>(res) == &s.s);
        assert(count == 2);
#if TEST_NUM == 202
        // const reference result modification attempt
        std::get<1>(res)++;
#endif
#if TEST_NUM == 203
        // result shouldn't be a range
        for (int x : res)
            count++;
#endif
    }
#endif
#if TEST_NUM == 204
    invoke_forall(std::tuple{}, std::array{1, 2});
#endif
#if TEST_NUM == 301
    // perfect forwarding tests
    {
        // no copy for temporal objects passed in tuple of size 1
        Tracer::id_counter = 0;
        auto res = invoke_forall(std::identity{}, std::tuple{Tracer{}});
        assert(std::get<0>(res).is(0));
    }
    {
        // forwarding plain arguments
        Tracer::id_counter = 0;
        Tracer a, b(a), c, d;
        auto res = invoke_forall(Tracer{}, a, b, std::move(c), static_cast<const Tracer&>(d));
        assert(std::get<0>(res).is(0));
        assert(std::get<1>(res).is(1, 0));
        assert(std::get<2>(res).is(2));
        assert(std::get<3>(res).is(3));
        assert(a.is(0));
        assert(b.is(1, 0));
        assert(c.is_moved());
        assert(d.is(3));
    }
    {
        // forwarding with tuple arguments
        Tracer::id_counter = 0;
        Tracer a, b, c;
        auto res = invoke_forall(
            std::tuple{
                [](Tracer& tr) -> Tracer& { return tr; },
                [](const Tracer& tr) { return &tr; },
                [](Tracer&& tr) { return std::move(tr); }},
            std::tuple<Tracer&, Tracer&, Tracer>{a, b, std::move(c)});
        assert(static_cast<Tracer&>(std::get<0>(res)).is(0));
        assert(std::get<1>(res) == &b);
        assert(std::get<2>(res).is(2));
        assert(a.is(0));
        assert(b.is(1));
        assert(c.is_moved());
    }
    {
        // arity > 1 scenario with plain temporal argument
        Tracer::id_counter = 0;
        assert(std::ranges::size(
            invoke_forall(
                    [](int x, Tracer&& tr) {
                        assert(tr.is_alive());
                        assert(x == tr.id % 3);
                        return std::monostate{};
                    },
                    std::array{1, 2, 0},
                    Tracer{}))
            == 3);
    }
    {
        // arity > 1 scenario with plain argument moved
        Tracer::id_counter = 0;
        Tracer a;
        assert(std::ranges::size(
            invoke_forall(
                [](int, Tracer& tr) {
                    assert(tr.is(0));
                    return std::monostate{};
                },
                std::array{1, 2, 0},
                a))
            == 3);
        assert(a.is(0));
    }
#endif
#if TEST_NUM == 402
    // similar return types shouldn't give range result
    {
        static int x = 43;
        auto res = invoke_forall(
            std::tuple{[] { return 42; }, [&] -> const int& { return x; }}
        );
        static_assert(!std::is_same_v<
            decltype(std::get<0>(res)),
            decltype(std::get<1>(res))>);
    }
    // custom tuple-like type test with arity 0 - optional test
    // static_assert(acmp(
    //     invoke_forall(
    //         [](std::size_t x) { return x * 2; },
    //         F<0>{}),
    //     std::array<int, 0>{}));
#endif
#if TEST_NUM == 403
    // tuple_size defined but not std::get<0>
    static_assert(invoke_forall(std::identity{}, F<1>{}) == F<1>{});
#endif
#if TEST_NUM == 501 || TEST_NUM == 404
    // protect_arg tests with tuple or array argument
    sassert(
        invoke_forall(
            []<typename T>(const T&) { return std::tuple_size_v<T>; },
            protect_arg(std::tuple{}))
        == 0);
    sassert(
        invoke_forall(
            []<typename T>(const T& x) { return std::get<0>(x); },
            protect_arg(std::tuple{5}))
        == 5);
    sassert(
        acmp(
            invoke_forall(
                [](auto f, int x, auto&& arr) {
                    return std::accumulate(arr.begin(), arr.end(), x, f);
                },
                std::tuple{std::plus{}, std::multiplies<int>{}},
                std::array{0, 1},
                protect_arg(std::array{1, 2, 3, 4})
            ),
            std::array{10, 24}
        ));
    sassert(
        tcmp([]{
            std::array a{1, 2, 3};
            int res =
                invoke_forall(
                    [](auto& arr) {
                        int sum = 0;
                        for (auto& x : arr)
                            sum += x++;
                        return sum;
                    },
                    protect_arg(a));
            return std::tuple{a, res}; }(),
        std::tuple{std::array{2, 3, 4}, 6}));
#endif
#if TEST_NUM == 502
    const std::array a{1, 2, 3};
    int res =
        invoke_forall(
            [](auto& arr) {
                int sum = 0;
                for (auto& x : arr)
                    sum += x++;
                return sum;
            },
            protect_arg(a));
    assert(res == 6);
#endif
#if TEST_NUM == 601
    // protect_arg tests with other argument types
    {
        std::tuple t{2, 3.25, std::string("abc")};
        auto dbl = [](auto& x) { x = x + x; };
        auto dbl_all = [=]<std::size_t... Is>(std::index_sequence<Is...>, auto&& r) {
            (dbl(std::get<Is>(r)), ...);
        };
        invoke_forall(
            [=](auto&& r) { dbl_all(std::make_index_sequence<3>{}, std::move(r)); },
            protect_arg(invoke_forall(std::identity{}, t))
        );
        assert(tcmp(t, std::tuple{4, 6.5, std::string("abcabc")}));
    }
    assert(invoke_forall(protect_arg(add), 3, 5) == 8);
    {
        int a[2] = {1, 2};
        const int b[2] = {3, 4};
        invoke_forall(
            [](int x[2], const int (&y)[2]) {
                for (int i = 0; i < 2; i++)
                    x[i] = y[i];
            },
            protect_arg(a), protect_arg(b));
        assert(a[0] == b[0] && a[1] == b[1]);
    }
#endif
}
