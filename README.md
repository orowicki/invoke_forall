# invoke_forall

## Overview

`invoke_forall` is a template utility that performs multiple `std::invoke` calls on a mix of regular and tuple-like (Gettable) arguments.

The library supports:
- Automatic expansion of tuple-like arguments across multiple invocations.
- Perfect forwarding and safe handling of rvalues.
- Optional protection of tuple-like arguments using protect_arg to treat them as regular arguments.
- Compile-time computation when possible, using constexpr.
- Returning results as std::array (if types are uniform) or std::tuple.

## Features

- Supports Gettable and non-Gettable arguments in the same call.
- Enforces consistent arity for tuple-like arguments.
- Works with functions, member functions, and callable objects.
- Maintains l-value references in results where applicable.
- Fully contained in a single header with internal helpers hidden in the detail namespace.

## Usage
```
#include "invoke_forall.h"
#include <tuple>
#include <iostream>

int add(int a, int b) { return a + b; }

int main() {
    auto t = std::make_tuple(1, 2, 3);
    auto results = invoke_forall(add, t, t); // calls add(1,1), add(2,2), add(3,3)
    
    for (auto r : results) std::cout << r << " ";
}
```

## Protecting arguments
```
auto protected_t = protect_arg(t); // treat t as a regular argument (eg. when callable takes a tuple as an argument)
```

## Requirements
Compiler support for concepts and constexpr evaluation

## Implementation notes
- Header-only; all helpers are in detail namespace.
- Uses concepts to enforce argument constraints (Gettable, SameArity, NonEmpty).
- Handles void return types gracefully by returning std::monostate.
- Supports `std::ranges::random_access_range` when result types are uniform.
- In case of result types uniformly being lvalue references, returns an array of `std::reference_wrapper`.
