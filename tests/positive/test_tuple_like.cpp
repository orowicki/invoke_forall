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

int main() {
    static_assert(detail::Gettable<std::pair<int, int>>);
    static_assert(!detail::Gettable<double>);

    static_assert(detail::HasTupleSize<BadTuple1>);
    static_assert(!detail::HasTupleSizeDerivedFromIntegralConstant<BadTuple1>);
    static_assert(!detail::Gettable<BadTuple1>);

    static_assert(detail::HasTupleSize<BadTuple2>);
    static_assert(!detail::HasTupleSizeDerivedFromIntegralConstant<BadTuple2>);
    static_assert(!detail::Gettable<BadTuple2>);
}
