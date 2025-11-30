#include "invoke_forall.h"
#include <list>
#include <string>
#include <tuple>

int main() {
    constexpr auto res = invoke_forall(
        [](std::string s, std::list<int> l) {
            return s.length() * l.size();
        },
        std::tuple{std::string("aa"), std::string("bbb")},
        protect_arg(std::list<int>{1, 2, 3, 4, 5})
    );

    static_assert(std::get<0>(res) == 10);
    static_assert(std::get<1>(res) == 15); // != 0
}
