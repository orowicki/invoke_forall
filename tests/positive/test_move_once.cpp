#include "invoke_forall.h"
#include <vector>
#include <string>
#include <tuple>

int main() {
    constexpr auto res1 = invoke_forall(
        [](std::string s1, std::string s2, std::string s3) {
            return s1.length() * s2.length() * s3.length();
        },
        std::string("aaa"),
        std::tuple{std::string("abacaba"), std::string("ab"), std::string("c")},
        std::array{std::string("bbab"), std::string("abca"), std::string("bb")}
    );

    static_assert(std::get<0>(res1) == 84);
    static_assert(std::get<1>(res1) == 24); // != 0
    static_assert(std::get<2>(res1) == 6); // != 0

    constexpr auto res2 = invoke_forall(
        [](std::string s, std::vector<int> v) {
            return s.length() * v.size();
        },
        std::tuple{std::string("aa"), std::string("bbb")},
        protect_arg(std::vector<int>{1, 2, 3, 4, 5})
    );

    static_assert(std::get<0>(res2) == 10);
    static_assert(std::get<1>(res2) == 15); // != 0
}
