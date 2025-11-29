#include "invoke_forall.h"
#include <array>
#include <functional>
#include <iostream>
#include <string>
#include <tuple>

namespace {
    int sum1(const std::array<int, 3> &t) {
        int res = 0;
        for (int x : t)
            res += x;
        return res;
    }

    int sum2(std::array<int, 3> &t, int d) {
        int res = 0;
        for (int &x : t) {
            x -= d;
            res += x;
        }
        return res;
    }

    template <std::size_t I>
    struct getter {
        template <typename T>
        constexpr decltype(auto) operator()(T &&t) const {
            return get<I>(std::forward<T>(t));
        }
    };

    constexpr auto test1() {
        return invoke_forall(
            std::tuple{getter<2>{}, getter<0>{}},
            protect_arg(std::tuple{10, 20.5, "hello"})
        );
    }
} // anonymous namespace

int main() {
    // no tuple-like arguments: simple invoke
    auto res1 = invoke_forall(std::plus<int>{}, 2, 3);
    // It writes out "res1 = 5\n".
    std::cout << "res1 = " << res1 << '\n';

    // tuple-like arguments: two arrays of same arity
    std::array<int, 3> A = {1, 2, 3};
    std::array<int, 3> B = {10, 20, 30};
    auto res2 = invoke_forall(std::plus<int>{}, A, B);
    // It writes out "res2 = 11 22 33\n".
    std::cout << "res2 =";
    for (auto const &x : res2)
        std::cout << ' ' << x;
    std::cout << '\n';

    // mixed arguments: call f(scalar, tuple)
    std::array<int, 3> C = {4, 5, 6};
    auto res3 = invoke_forall([](int s, int t){ return s * t; }, 10, C);
    // It writes out "res3 = 40 50 60\n".
    std::cout << "res3 =";
    for (auto const &x : res3)
        std::cout << ' ' << x;
    std::cout << '\n';

    // tuple-like heterogeneous arguments
    std::tuple<int, double, std::string> T1 = {1, 2.5, std::string("abc")};
    auto res4 = invoke_forall([](auto a){ return a; }, T1);
    // It writes out "res4 = 1 2.5 abc\n".
    std::cout << "res4 = "
              << get<0>(res4) << " " << get<1>(res4) << " " << get<2>(res4)
              << '\n';

    // It writes out "sum1 = 6\n".
    std::cout << "sum1 = "
              << invoke_forall(sum1, protect_arg(std::array{1, 2, 3}))
              << '\n';

    // It writes out "sum2 = 12 6 -3\n".
    std::cout << "sum2 =";
    for (int x : invoke_forall(sum2, protect_arg(C), A))
        std::cout << ' ' << x;
    std::cout << '\n';

    // It writes out "sum3 = -3\n".
    std::cout << "sum3 = "
              << invoke_forall(sum1, protect_arg(C))
              << '\n';

    // void return type test
    // auto z = std::get<0>(invoke_forall([](int &x){ x += 5; }, A));
    // The behavior of the above line is undefined – it may not compile.
    invoke_forall([](int &x){ x += 5; }, A);
    // It writes out "A = 6 7 8\n".
    std::cout << "A =";
    for (int x : A)
        std::cout << ' ' << x;
    std::cout << '\n';

    // code evaluated at compile time
    static_assert(91 == invoke_forall(std::multiplies<int>{}, 7, 13));
    constexpr auto t = test1();
    static_assert(std::get<0>(t) == std::string("hello"));
    static_assert(std::get<1>(t) == 10);
}
