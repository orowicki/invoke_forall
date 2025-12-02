#include "invoke_forall.h"
#include <cassert>
#include <list>
#include <string>
#include <tuple>

int main() {
    auto res = invoke_forall(
        [](std::string s, std::list<int> l) {
            return s.length() * l.size();
        },
        std::tuple{std::string("abc"), std::string("bbab")},
        std::list<int>{1, 2, 3, 4, 5}
    );

    assert(std::get<0>(res) == 15);
    assert(std::get<1>(res) == 20); // != 0
}
