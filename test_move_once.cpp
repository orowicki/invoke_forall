#include "invoke_forall.h"
#include <string>
#include <tuple>

int main() {
    constexpr auto res = invoke_forall(
        [](std::string s1, std::string s2){ return s1.length() * s2.length(); },
        std::string("aaa"),
        std::tuple{std::string("abacaba"), std::string("ab"), std::string("c")}
    );

    static_assert(std::get<0>(res) == 21);
    static_assert(std::get<1>(res) == 6); // != 0
    static_assert(std::get<2>(res) == 3); // != 0
}
