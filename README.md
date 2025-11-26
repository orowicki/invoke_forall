# Wielokrotkowe wołanie funkcji

## Wstęp

Studenci powinni poznać:

- szablony oraz ich specjalizację i częściową specjalizację;
- techniki manipulowania typami i danymi na etapie kompilowania programu;
- poznać `constexpr` i dowiedzieć się, że w kolejnych standardach coraz więcej elementów z biblioteki standardowej ma takie oznaczenie, np. `std::max` od C++14;
- techniki radzenia sobie z szablonami o zmiennej liczbie argumentów;
- elementy biblioteki standardowej pomagające w powyższym, np. `type_traits`;
- poznać ideę perfect forwarding (użycie `&&`, `std::forward`);
- znaczenie `decltype(auto)`;
- podstawowe informacje o konceptach.

# Polecenie

W bibliotece standardowej istnieje szablon funkcji `std::invoke` służący m.in. do wywoływania funkcji z podanymi argumentami. Należy zaimplementować szablon funkcji `invoke_forall`, który będzie wielokrotnie wywoływał `std::invoke`. Deklaracja tego szablonu powinna wyglądać tak:

```cpp
template<typename... Args> constexpr typ_wyniku invoke_forall(Args&&... args);
```

gdzie `typ_wyniku` będzie zależał od typów argumentów. Implementacja powinna wymuszać za pomocą konceptu, że jest co najmniej jeden argument.

W C++11 zostało wprowadzone pojęcie typu krotkowego (ang. *tuple-like*). Mówimy, że typ `T` jest *krotkowy*, jeżeli zdefiniowana jest dla niego specjalizacja `std::tuple_size<T>` dziedzicząca z `std::integral_constant`. Arność takiego typu określa `std::tuple_size_v<T>`.

Mówimy, że argument `arg` jest *wyciągalny* (ang. *gettable*), jeżeli jego goły typ `T` (czyli typ bez specyfikatorów `const` i `volatile` oraz bez referencji `&` i `&&`) jest krotkowy i można zaaplikować do tego argumentu funkcję `std::get<i>` dla `i = 0, ..., std::tuple_size_v<T> - 1`.

Jeżeli żaden argument wywołania `invoke_forall(arg1, ..., argn)` nie jest wyciągalny, to takie wywołanie powinno być równoważne z wywołaniem `std::invoke(arg1, ..., argn)`. W przeciwnym przypadku wszystkie wyciągalne argumenty powinny mieć tę samą arność (oznaczmy ją przez `m`) i powinno zostać wykonanych `m` wywołań `std::invoke`. Wtedy `i`-te wywołanie (`i = 0, ..., m - 1`) powinno przekazać do `std::invoke std::get<i>`, jeśli odpowiedni argument jest wyciągalny, a oryginalny argument, jeśli odpowiedni argument nie jest wyciągalny. W wyniku powinien zostać zwrócony pewien obiekt `res`, taki że `std::get<i>(res)` będzie wynikiem `i`-tego wywołania `std::invoke`. Jeżeli wynik `i`-tego wywołania jest typu `void`, to zachowanie `std::get<i>(res)` jest niezdefiniowane. Dodatkowo, jeżeli typy wyników wszystkich wywołań `std::invoke` są takie same, to wynik `invoke_forall` powinien spełniać koncept `std::ranges::random_access_range`.

Powyższa specyfikacja `invoke_forall` nie pozwala przekazać obiektu, który jest wyciągalny, bezpośrednio do `std::invoke`. Aby to umożliwić, dodatkowo należy zaimplementować następujący szablon funkcji:

```cpp
template <typename T> constexpr typ protect_arg(T&&);
```

gdzie `typ` jest też do zaprojektowania. Jeżeli `j`-ty argument wywołania `invoke_forall` ma postać `protect_arg(argj)`, to w każdym wywołaniu `std::invoke` argumentem `j`-tym powinno być `argj` z zachowaniem *perfect forwarding*.

**Uwaga.** Jeżeli argumenty wywołania `invoke_forall` implikują co najmniej dwukrotne wywołanie `std::invoke` i istnieje zwykły argument (niekrotkowy), który jest r-wartością, to należy pamiętać, że r-wartość można przekazać dalej tylko raz, więc w pozostałych wywołaniach niezbędne jest utworzenie kopii tego argumentu przed przekazaniem go do `std::invoke`. Zakładamy, że taki argument ma zdefiniowany zarówno konstruktor kopiujący, jak i przenoszący.

# Wymagania

- Należy zastosować zasadę *perfect forwarding* przy wykonywania `std::invoke`.
- Jeżeli wszystkie wywołania `std::invoke` dadzą się policzyć w czasie kompilowania, to całe `invoke_forall` też powinno.
- Pomocnicze definicje należy ukryć przed światem. W związku z tym, że rozwiązanie jest w pliku nagłówkowym, definicje ukrywamy w przestrzeni nazw o nazwie na przykład `detail`.
- Jeżeli wynik `i`-tego wywołania `std::invoke` zwraca l-referencję, to powinno być możliwe modyfikowanie obiektu `std::get<i>(res)`.

# Wymagania formalne

Rozwiązanie należy umieścić w pliku `invoke_forall.h`, który należy wstawić do Moodle. Rozwiązanie będzie kompilowane na maszynie students poleceniem

```shell
clang++ -Wall -Wextra -std=c++23 -O2 *.cpp
```
