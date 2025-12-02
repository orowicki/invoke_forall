#include "invoke_forall.h"
#include <string>
#include <tuple>
#include <vector>

int main() {
    constexpr auto res1 = invoke_forall(
        [](std::string s, std::vector<int> v) {
            return s.length() * v.size();
        },
        std::tuple{std::string("aa"), std::string("bbb")},
        std::vector<int>{1, 2, 3, 4, 5}
    );

    static_assert(std::get<0>(res1) == 10);
    static_assert(std::get<1>(res1) == 15);

        constexpr auto res2 = invoke_forall(
        [](std::string s, std::vector<int> v) {
            return s.length() * v.size();
        },
        std::tuple{std::string("aa"), std::string("bbb")},
        protect_arg(std::vector<int>{1, 2, 3, 4, 5})
    );

    static_assert(std::get<0>(res2) == 10);
    static_assert(std::get<1>(res2) == 15);
}
