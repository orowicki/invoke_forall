#include "invoke_forall.h"
#include <utility>

struct BadTuple1 {
    int a;
    double b;
};

struct BadTuple2 {
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
    struct tuple_size<BadTuple2> {
        using type = void;
        static constexpr std::size_t value = 2;
    };
    
    template <>
    struct tuple_size<GoodTuple> : std::integral_constant<std::size_t, 3> {};
}

int main() {
    static_assert(detail::TupleLike<std::pair<int, int>>);
    static_assert(!detail::TupleLike<double>);

    static_assert(detail::HasTupleSize<BadTuple1>);
    static_assert(!detail::HasTupleSizeDerivedFromIntegralConstant<BadTuple1>);
    static_assert(!detail::TupleLike<BadTuple1>);

    static_assert(detail::HasTupleSize<BadTuple2>);
    static_assert(!detail::HasTupleSizeDerivedFromIntegralConstant<BadTuple2>);
    static_assert(!detail::TupleLike<BadTuple2>);

    static_assert(detail::HasTupleSize<GoodTuple>);
    static_assert(detail::HasTupleSizeDerivedFromIntegralConstant<GoodTuple>);
    static_assert(detail::TupleLike<GoodTuple>);
}
