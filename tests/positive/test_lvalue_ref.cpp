#include "invoke_forall.h"
#include <array>
#include <cassert>
#include <functional>
#include <ranges>
#include <tuple>

bool test_runtime() {
    int a = 10;
    int b = 20;
    
    decltype(auto) res1 = invoke_forall(
        [](int& i) -> int& { return i; },
        std::tie(a, b)
    );
    
    static_assert(std::ranges::random_access_range<decltype(res1)>);
    
    if (std::get<0>(res1) != 10) { return false; }
    if (std::get<1>(res1) != 20) { return false; }

    std::get<0>(res1) = 30;
    if (a != 30) { return false; }

    std::get<0>(res1) = std::get<1>(res1) = 50;
    if (a != 50) { return false; }
    if (b != 50) { return false; }

    int c = 15;
    decltype(auto) res2 = invoke_forall([](int& i) -> int& { return i; }, std::ref(c));
    if (res2 != 15) { return false; }
    res2 = 5;
    if (c != 5) { return false; }

    return true;
}

constexpr bool test_constexpr() {
    int a = 10;
    int b = 20;
    
    decltype(auto) res1 = invoke_forall(
        [](int& i) -> int& { return i; },
        std::tie(a, b)
    );
    
    static_assert(std::ranges::random_access_range<decltype(res1)>);

    if (std::get<0>(res1) != 10) { return false; }
    if (std::get<1>(res1) != 20) { return false; }

    std::get<0>(res1) = 30;
    if (a != 30) { return false; }

    std::get<0>(res1) = std::get<1>(res1) = 50;
    if (a != 50) { return false; }
    if (b != 50) { return false; }

    int c = 15;
    decltype(auto) res2 = invoke_forall([](int& i) -> int& { return i; }, std::ref(c));
    if (res2 != 15) { return false; }
    res2 = 5;
    if (c != 5) { return false; }

    return true;
}

int main() {
    assert(test_runtime());
    static_assert(test_constexpr());
}