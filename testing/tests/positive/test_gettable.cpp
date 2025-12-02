#include <utility>

struct BadTuple3;
struct GoodTuple;

namespace std {
    template <std::size_t I>
    decltype(auto) get(BadTuple3& t);

    template <std::size_t I>
    decltype(auto) get(GoodTuple& t);
};

#include "invoke_forall.h"

struct BadTuple1 {
    int a;
    double b;
};

struct BadTuple2 {
    int a;
    double b;
};

struct BadTuple3 {
    int a;
    double b;
};

struct GoodTuple {
    int a;
    double b;
    float c;
};

namespace std {
    template <typename T>
    struct tuple_size;
    
    template <>
    struct tuple_size<BadTuple1> {
        using type = void;
    };
    
    template <>
    struct tuple_size<BadTuple2> : std::integral_constant<std::size_t, 2> {};

    template <>
    struct tuple_size<BadTuple3> : std::integral_constant<std::size_t, 2> {};    
    
    template <std::size_t I>
        requires (I == 0)
    decltype(auto) get(BadTuple3& t) {
        return t.a;
    };

    template <>
    struct tuple_size<GoodTuple> : std::integral_constant<std::size_t, 3> {};

    template <std::size_t I>
    decltype(auto) get(GoodTuple& t) {
        if constexpr (I == 0) return t.a;
        if constexpr (I == 1) return t.b;
        if constexpr (I == 2) return t.c;
    }
}

int main() {
    static_assert(detail::Gettable<std::pair<int, int>>);
    static_assert(!detail::Gettable<double>);

    static_assert(!detail::TupleLike<BadTuple1>);
    static_assert(!detail::Gettable<BadTuple1>);

    static_assert(detail::TupleLike<BadTuple2>);
    static_assert(!detail::Gettable<BadTuple2>);

    static_assert(detail::TupleLike<BadTuple3>);
    static_assert(!detail::Gettable<BadTuple3>);

    static_assert(detail::TupleLike<GoodTuple>);
    static_assert(detail::Gettable<GoodTuple>);
}
