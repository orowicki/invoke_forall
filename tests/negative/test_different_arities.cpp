#include "invoke_forall.h"
#include <array>
#include <string>
#include <tuple>

int main() {
    constexpr auto res = invoke_forall(
        [](std::string s1, std::string s2, std::string s3) {
            return s1.length() * s2.length() * s3.length();
        },
        std::string("aaa"),
        std::tuple{std::string("abacaba"), std::string("ab"), std::string("c")},
        std::array{std::string("bbab"), std::string("abca")}
    );
}
